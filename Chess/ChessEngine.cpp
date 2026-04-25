#include "ChessEngine.h"

#include "ImageHelper.h"

#define PADDING (ImVec2(50.f, 50.f))

#define COL_BG IM_COL32(50, 50, 50, 255)
#define COL_BLACK IM_COL32(118, 150, 86, 255)
#define COL_WHITE IM_COL32(231, 231, 204, 255)
#define COL_ALLOWEDMOVE IM_COL32(0, 0, 0, 50)
#define COL_MARKED_SQUARE IM_COL32(255, 0, 0, 127)
#define COL_SELECTED_SQUARE IM_COL32(255, 255, 0, 127)

ChessEngine::ChessEngine()
	: SelectedPiece(nullptr)
{
	int ImageWidth = 0;
	int ImageHeight = 0;

	bool bSuccess = LoadTextureFromFile("../Assets/Chess_Pieces_Sprite.png", &ImageTexture, &ImageWidth, &ImageHeight);

	if (!bSuccess)
	{
		bSuccess = LoadTextureFromFile("../../Assets/Chess_Pieces_Sprite.png", &ImageTexture, &ImageWidth, &ImageHeight);
	}

	IM_ASSERT(bSuccess && "Failed to load pieces texture!");

	LoadFENPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	// LoadFENPosition("r1bqkbnr/pppp1ppp/2n5/4p3/1b1PP3/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 2 4");
	// LoadFENPosition("qqqqqqqq/qqqqqqqq/8/8/8/8/QQQQQQQQ/QQQQQQQQ w KQkq - 0 1");
	// LoadFENPosition(NULL);
}

void ChessEngine::LoadFENPosition(const char* FENString)
{
	Pieces.clear();

	SelectedPieceMouseOffset = {};
	SelectedPiece = nullptr;

	CurrentMove = White;
	EnpassantSquare = -1;
	HalfMoveClock = 0;
	FullMoveCounter = 1;

	if (FENString == nullptr || strlen(FENString) == 0)
	{
		return; // allow option to clear board
	}

	int MinRank = 0;
	int MaxRank = 7;

	int MinFile = 0;
	int MaxFile = 7;

	int CurrentRank = MaxRank; // top to bottom
	int CurrentFile = MinFile; // left to right

	bool bSetupPlacement = false;
	bool bSetupCurrentMove = false;
	bool bSetupCastling = false;
	bool bSetupEnpassant = false;
	bool bSetupHalfMoveClock = false;
	bool bSetupFullMoveCounter = false;

	while (char c = *FENString++)
	{
		if (c == '/')
		{
			CurrentRank--;
			CurrentFile = MinFile;
		}
		else if (c == '-')
		{
			if (!bSetupCastling)
			{
				// no castling available (TODO)

				bSetupCastling = true;
			}
			else if (!bSetupEnpassant)
			{
				EnpassantSquare = -1;
				bSetupEnpassant = true;
			}
		}
		else if (isspace(c))
		{
			bSetupPlacement = true;
		}
		else if (isdigit(c))
		{
			if (!bSetupPlacement)
			{
				CurrentFile += c - '0' - 1;
			}
			else if (!bSetupHalfMoveClock)
			{
				HalfMoveClock = c - '0';

				bSetupHalfMoveClock = true;
			}
			else if (!bSetupFullMoveCounter)
			{
				FullMoveCounter = c - '0';

				bSetupFullMoveCounter = true;
			}
		}
		else if (isalpha(c))
		{
			if (!bSetupPlacement)
			{
				PieceColor Color = isupper(c) ? White : Black;
				PieceType Type;

				switch (tolower(c))
				{
				case 'r':
					Type = Rook;
					break;
				case 'n':
					Type = Knight;
					break;
				case 'b':
					Type = Bishop;
					break;
				case 'q':
					Type = Queen;
					break;
				case 'k':
					Type = King;
					break;
				case 'p':
					Type = Pawn;
					break;
				default:
					IM_ASSERT(false);
					break;
				}

				int Square = Piece::RankFileToSquare(Piece::RotateCW({ CurrentRank, CurrentFile++ }));

				Pieces.push_back(std::make_unique<Piece>(Square, Type, Color));
			}
			else if (!bSetupCurrentMove)
			{
				IM_ASSERT(c == 'w' || c == 'b');

				CurrentMove = c == 'w' ? White : Black;

				bSetupCurrentMove = true;
			}
			else if (!bSetupCastling)
			{
				// castling stuff (TODO)

				while (!isspace(*FENString++));

				bSetupCastling = true;
			}
			else if (!bSetupEnpassant)
			{
				std::array<char, 2> AlgebraicNotation = {
					c, *FENString++ // we are fine skipping the next one
				};

				int Square = Piece::RankFileToSquare(Piece::AlgebraicToRankFile(AlgebraicNotation));

				EnpassantSquare = Square;

				bSetupEnpassant = true;
			}
		}
	}

	CapturedPiece = Pieces.end();

	IM_ASSERT(bSetupPlacement && bSetupCurrentMove && bSetupCastling && bSetupEnpassant && bSetupHalfMoveClock && bSetupFullMoveCounter);
}

void ChessEngine::IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const
{
	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareRadius = (Radius - PADDING) / 4;

	for (int Rank = 0; Rank < 8; Rank++)
	{
		for (int File = 0; File < 8; File++)
		{
			ImVec2 x = Center + (Rank - 4) * SquareRadius;
			ImVec2 y = Center + (File - 4) * SquareRadius;

			ImVec2 SquareMin = ImVec2(x.x, y.y);
			ImVec2 SquareMax = SquareMin + SquareRadius;

			if (!Predicate(SquareMin, SquareMax, Rank, File))
			{
				return;
			}
		}
	}
}

bool ChessEngine::IsAllowedMove(int NewSquare, decltype(Pieces)::const_iterator* OutCapturedPiece, int* OutEnpassantSquare) const
{
#ifdef _DEBUG
	std::chrono::steady_clock::time_point IsAllowedMoveStart = std::chrono::high_resolution_clock::now();
#endif

	IM_ASSERT(NewSquare >= 0 && NewSquare <= 63);

	auto RotateFunction = SelectedPiece->Color == White ? Piece::RotateCCW : Piece::RotateCW;
	auto RotateBackFunction = SelectedPiece->Color == White ? Piece::RotateCW : Piece::RotateCCW;

	auto [OldRank, OldFile] = RotateFunction(Piece::SquareToRankFile(SelectedPiece->Square));
	auto [NewRank, NewFile] = RotateFunction(Piece::SquareToRankFile(NewSquare));

	int DeltaRank = NewRank - OldRank;
	int DeltaFile = NewFile - OldFile;

	int AbsDeltaRank = abs(DeltaRank);
	int AbsDeltaFile = abs(DeltaFile);

	int DeltaRankDir = AbsDeltaRank == 0 ? 0 : DeltaRank / AbsDeltaRank;
	int DeltaFileDir = AbsDeltaFile == 0 ? 0 : DeltaFile / AbsDeltaFile;

	bool bIsMoveAllowed = SelectedPiece->IsAllowedMove(NewSquare);

	decltype(Pieces)::const_iterator CapturedPiece = GetPieceIt(NewSquare);

	if (OutCapturedPiece)
	{
		*OutCapturedPiece = CapturedPiece;
	}

	bool bHasPiece = CapturedPiece != Pieces.end();
	bool bHasPieceOfSameColor = false;

	if (bHasPiece)
	{
		if ((*CapturedPiece)->Color == SelectedPiece->Color)
		{
			bHasPiece = false;
			bHasPieceOfSameColor = true;

			bIsMoveAllowed = false;
		}
	}

	bool bAllowEnpassant = false;

	if (SelectedPiece->Type == Pawn)
	{
		if (AbsDeltaRank == 1 && AbsDeltaFile == 1)
		{
			if (bHasPiece) // normal pawn capture
			{
				bIsMoveAllowed = true;
			}
			else if (EnpassantSquare != -1 && DeltaRank > 0) // en passant capture
			{
				int EnpassantSquareToTest = Piece::RankFileToSquare(RotateBackFunction({ OldRank, OldFile + DeltaFile }));

				if (EnpassantSquareToTest == EnpassantSquare)
				{
					decltype(Pieces)::const_iterator EnpassantPieceIt = GetPieceIt(EnpassantSquare);

					bHasPiece = EnpassantPieceIt != Pieces.end();

					if (bHasPiece)
					{
						if ((*EnpassantPieceIt)->Color != SelectedPiece->Color &&
							(*EnpassantPieceIt)->Type == Pawn)
						{
							bIsMoveAllowed = true;

							if (OutCapturedPiece)
							{
								*OutCapturedPiece = EnpassantPieceIt;
							}
						}
					}
				}
			}
		}
		else if (AbsDeltaRank == 2 && AbsDeltaFile == 0 && OldRank == 1 && !bHasPiece && !bHasPieceOfSameColor) // pawn's first move
		{
			bIsMoveAllowed = true;

			if (OutEnpassantSquare)
			{
				*OutEnpassantSquare = NewSquare;
			}

			bAllowEnpassant = true;
		}
		else if (bHasPiece) // pawn getting blocked
		{
			bIsMoveAllowed = false;
		}
	}
	else if (SelectedPiece->Type == Bishop || SelectedPiece->Type == Rook || SelectedPiece->Type == Queen)
	{
		bool bRayHitPiece = false;

		auto CheckSquare = [&bIsMoveAllowed, &bRayHitPiece, this](int RankToCheck, int FileToCheck) -> bool
			{
				if (bRayHitPiece)
				{
					bIsMoveAllowed = false;
					return true;
				}

				if (Piece* SkippedPiece = GetPiece(RankToCheck, FileToCheck))
				{
					if (SkippedPiece->Color == SelectedPiece->Color)
					{
						bIsMoveAllowed = false;
						return true;
					}

					bRayHitPiece = true;
				}

				return false;
			};

		if (AbsDeltaRank == AbsDeltaFile) // diagonal move
		{
			for (int i = 1; i < AbsDeltaRank + 1; i++)
			{
				int CurrentRank = OldRank + i * DeltaRankDir;
				int CurrentFile = OldFile + i * DeltaFileDir;

				if (CurrentRank < 0 || CurrentRank > 7 ||
					CurrentFile < 0 || CurrentFile > 7)
				{
					break;
				}

				const auto [RankToCheck, FileToCheck] = RotateBackFunction({ CurrentRank, CurrentFile });

				if (CheckSquare(RankToCheck, FileToCheck))
				{
					break;
				}

				CurrentRank += DeltaRankDir;
			}
		}
		else if (DeltaFile == 0)
		{
			for (int i = 1; i < AbsDeltaRank + 1; i++)
			{
				int CurrentRank = OldRank + i * DeltaRankDir;

				if (CurrentRank < 0 || CurrentRank > 7)
				{
					break;
				}

				const auto [RankToCheck, FileToCheck] = RotateBackFunction({ CurrentRank, NewFile });

				if (CheckSquare(RankToCheck, FileToCheck))
				{
					break;
				}

				CurrentRank += DeltaRankDir;
			}
		}
		else if (DeltaRank == 0)
		{
			for (int i = 1; i < AbsDeltaFile + 1; i++)
			{
				int CurrentFile = OldFile + i * DeltaFileDir;

				if (CurrentFile < 0 || CurrentFile > 7)
				{
					break;
				}

				const auto [RankToCheck, FileToCheck] = RotateBackFunction({ NewRank, CurrentFile });

				if (CheckSquare(RankToCheck, FileToCheck))
				{
					break;
				}

				CurrentFile += DeltaFileDir;
			}
		}
	}
	else if (SelectedPiece->Type == King)
	{
		// CASTLING: TODO
	}

	if (!bAllowEnpassant && bIsMoveAllowed)
	{
		if (OutEnpassantSquare)
		{
			*OutEnpassantSquare = -1;
		}
	}

	if (!bIsMoveAllowed)
	{
		if (OutCapturedPiece)
		{
			*OutCapturedPiece = Pieces.end();
		}
	}

#ifdef _DEBUG
	std::chrono::steady_clock::time_point IsAllowedMoveEnd = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds IsAllowedMoveDuration = std::chrono::duration_cast<std::chrono::microseconds>(IsAllowedMoveEnd - IsAllowedMoveStart);

	// printf(__FUNCTION__" took %lld us\n", IsAllowedMoveDuration.count());
#endif

	return bIsMoveAllowed;
}

void ChessEngine::FinishMove()
{
	if (CurrentMove == Black)
	{
		FullMoveCounter++;
	}

	if (SelectedPiece->Type == Pawn)
	{
		HalfMoveClock = 0;
	}

	CurrentMove = (PieceColor)(1 - CurrentMove);
}

void ChessEngine::SnapSelectedPiece(const ImVec2& Pos)
{
	if (!SelectedPiece)
	{
		return;
	}

	int NewSquare = GetSquare(Pos);

	if (NewSquare < 0 || NewSquare > 63)
	{
		return;
	}

	if (SelectedPiece->Square == NewSquare)
	{
		return; // no move happened
	}

	bool bIsMoveAllowed = IsAllowedMove(NewSquare, &CapturedPiece, &EnpassantSquare);

	if (bIsMoveAllowed)
	{
		LastMove.OldSquare = SelectedPiece->Square;
		LastMove.NewSquare = NewSquare;

		SelectedPiece->Square = NewSquare;
		HalfMoveClock++; // this *might* get set to zero in one of the following two calls

		if (CapturedPiece != Pieces.end())
		{
			CapturePiece();
		}

		FinishMove();
	}
}

void ChessEngine::UpdateSelectedPiece(const ImVec2& Pos)
{
	int OldSquare = SelectedPiece ? SelectedPiece->Square : -1;

	SnapSelectedPiece(Pos);

	int NewSquare = SelectedPiece ? SelectedPiece->Square : -1;

	SelectedPieceMouseOffset = { -FLT_MAX, FLT_MAX };

	if (OldSquare != NewSquare) // only null it if we actually moved
	{
		SelectedPiece = nullptr;
	}
}

void ChessEngine::SelectPiece(const ImVec2& Pos)
{
	bool bFoundPiece = SelectedPiece != nullptr;

	IterateSquares([Pos, &bFoundPiece, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			if (Pos.x >= Min.x && Pos.x <= Max.x &&
				Pos.y >= Min.y && Pos.y <= Max.y)
			{
				if (SelectedPiece = GetPiece(Rank, File))
				{
					SelectedPieceMouseOffset = Pos - Min;
				}

				bFoundPiece = SelectedPiece != nullptr;
				return false;
			}

			return true;
		});

	if (bFoundPiece)
	{
		if (SelectedPiece && SelectedPiece->Color != CurrentMove)
		{
			SelectedPiece = nullptr;
		}
	}
	else
	{
		SelectedPiece = nullptr;
	}
}

void ChessEngine::HandleInput()
{
	const ImVec2 MousePos = ImGui::GetMousePos();

	bool bLeftMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool bLeftMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

	bool bRightMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

	if (bLeftMouseClicked || bLeftMouseReleased)
	{
		UpdateSelectedPiece(MousePos);

		if (bLeftMouseClicked)
		{
			SelectPiece(MousePos);
		}
	}
	else if (bRightMouseClicked)
	{
		SelectedPieceMouseOffset = { -FLT_MAX, FLT_MAX };
		SelectedPiece = nullptr;
	}
}

void ChessEngine::CapturePiece()
{
	HalfMoveClock = 0;
	Pieces.erase(CapturedPiece);
}

Piece* ChessEngine::GetPiece(int Rank, int File) const
{
	int Square = Piece::RankFileToSquare(Rank, File);

	return GetPiece(Square);
}

Piece* ChessEngine::GetPiece(int Square) const
{
	if (Square < 0 || Square > 63)
	{
		return nullptr;
	}

	for (const auto& Piece : Pieces)
	{
		if (Piece->Square == Square)
		{
			return Piece.get();
		}
	}

	return nullptr;
}

int ChessEngine::GetSquare(const ImVec2& Pos) const
{
	int Square = -1;

	IterateSquares([Pos, &Square](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			if (Pos.x >= Min.x && Pos.x <= Max.x &&
				Pos.y >= Min.y && Pos.y <= Max.y)
			{
				Square = Piece::RankFileToSquare(Rank, File);
				return false;
			}

			return true;
		});

	return Square;
}

std::pair<ImVec2, ImVec2> ChessEngine::GetSquare(std::pair<int, int> RankFile) const
{
	const ImVec2 MousePos = ImGui::GetMousePos();

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareRadius = (Radius - PADDING) / 4;

	const auto [Rank, File] = RankFile;

	ImVec2 X = Center + (Rank - 4) * SquareRadius;
	ImVec2 Y = Center + (File - 4) * SquareRadius;

	ImVec2 Min = ImVec2(X.x, Y.y);
	ImVec2 Max = Min + SquareRadius;

	return { Min, Max };
}

std::pair<ImVec2, ImVec2> ChessEngine::GetSquare(int Square) const
{
	return GetSquare(Piece::SquareToRankFile(Square));
}

decltype(ChessEngine::Pieces)::const_iterator ChessEngine::GetPieceIt(int Rank, int File) const
{
	int Square = Piece::RankFileToSquare(Rank, File);

	if (Square < 0 || Square > 63)
	{
		return Pieces.end();
	}

	return GetPieceIt(Square);
}

decltype(ChessEngine::Pieces)::const_iterator ChessEngine::GetPieceIt(int Square) const
{
	if (Square < 0 || Square > 63)
	{
		return Pieces.end();
	}

	return std::find_if(Pieces.begin(), Pieces.end(), [Square](const auto& Piece) -> bool
		{
			return Piece->Square == Square;
		});
}

void ChessEngine::DrawPiece(const ImVec2& Pos, PieceType Type, PieceColor Color) const
{
	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareRadius = (Radius - PADDING) / 4;

	ImVec2 p_min = Pos;
	ImVec2 uv_min = ImVec2((float)Type		 / 6.f,	(float)Color	   / 2.f);

	ImVec2 p_max = Pos + SquareRadius;
	ImVec2 uv_max = ImVec2((float)(Type + 1) / 6.f, (float)(Color + 1) / 2.f);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->AddImage((ImTextureID)ImageTexture, p_min, p_max, uv_min, uv_max);
}

void ChessEngine::DrawPiece(int Square, PieceType Type, PieceColor Color) const
{
	IM_ASSERT(Square >= 0 && Square <= 63);

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareRadius = (Radius - PADDING) / 4;

	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	ImVec2 x = Center + (Rank - 4) * SquareRadius;
	ImVec2 y = Center + (File - 4) * SquareRadius;

	return DrawPiece(ImVec2(x.x, y.y), Type, Color);
}

void ChessEngine::DrawPiece(const Piece& Piece) const
{
	return DrawPiece(Piece.Square, Piece.Type, Piece.Color);
}

#ifdef _DEBUG
void ChessEngine::DrawMarkedSquares() const
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	IterateSquares([DrawList, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int ColorOffset = Rank % 2;
			int ColorSwitch = (File + ColorOffset) % 2;

			ImU32 Color = COL_MARKED_SQUARE;

			int Square = Piece::RankFileToSquare(Rank, File);

			if (std::find(MarkedSquares.begin(), MarkedSquares.end(), Square) != MarkedSquares.end())
			{
				DrawList->AddRectFilled(Min, Max, Color);
			}

			return true;
		});
}
#endif

void ChessEngine::DrawGrid() const
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 Min = Center - Radius;
	const ImVec2 Max = Center + Radius;

	DrawList->AddRectFilled(Min, Max, COL_BG);

	IterateSquares([DrawList, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int ColorOffset = Rank % 2;
			int ColorSwitch = (File + ColorOffset) % 2;

			ImU32 Color = ColorSwitch ? COL_BLACK : COL_WHITE;

			DrawList->AddRectFilled(Min, Max, Color);

#ifdef _DEBUG
			ImGui::SetCursorScreenPos(Min);
			ImGui::TextColored(ImVec4(0.f, 0.f, 0.f, 1.f), Piece::RankFileToAlgebraic({ Rank, File }).data());
#endif

			return true;
		});
}

void ChessEngine::DrawSelectedPiece() const
{
	if (!SelectedPiece)
	{
		return;
	}

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		const ImVec2 MousePos = ImGui::GetMousePos();

		DrawList->ChannelsSplit(2);
		DrawList->ChannelsSetCurrent(1);

		DrawPiece(MousePos - SelectedPieceMouseOffset, SelectedPiece->Type, SelectedPiece->Color);

		DrawList->ChannelsSetCurrent(0);
	}

	if (SelectedPiece->Square != LastMove.OldSquare && SelectedPiece->Square != LastMove.NewSquare)
	{
		const auto [Min, Max] = GetSquare(Piece::SquareToRankFile(SelectedPiece->Square));

		DrawList->AddRectFilled(Min, Max, COL_SELECTED_SQUARE);
	}
}

void ChessEngine::DrawMoveInfo() const
{
	// CurrentMove

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	int CurrentMoveRank = -2;
	int CurrentMoveFile = 0;

	const auto [CurrentMoveSquareMin, CurrentMoveSquareMax] = GetSquare(std::pair{ CurrentMoveRank, CurrentMoveFile });

	ImU32 CurrentColor = CurrentMove == White ? IM_COL32_WHITE : IM_COL32_BLACK;

	DrawList->AddRectFilled(CurrentMoveSquareMin - PADDING, CurrentMoveSquareMax + PADDING, COL_BG);
	DrawList->AddRectFilled(CurrentMoveSquareMin, CurrentMoveSquareMax, CurrentColor);

	// EnpassantSquare

	if (EnpassantSquare != -1)
	{
		const auto [EnpassantSquareMin, EnpassantSquareMax] = GetSquare(Piece::SquareToRankFile(EnpassantSquare));

		DrawList->AddRectFilled(EnpassantSquareMin, EnpassantSquareMax, IM_COL32(0, 255, 0, 127));
	}

	// HalfMoveClock

	ImGui::SetCursorScreenPos(ImVec2(CurrentMoveSquareMin.x - PADDING.x, CurrentMoveSquareMax.y + PADDING.y));
	ImGui::Text("HalfMoveClock: %d", HalfMoveClock);

	// FullMoveCounter

	ImGui::SetCursorScreenPos(ImVec2(CurrentMoveSquareMin.x - PADDING.x, ImGui::GetCursorPosY()));
	ImGui::Text("FullMoveCounter: %d", FullMoveCounter);

	// LastMove

	if (LastMove.OldSquare != -1 && LastMove.NewSquare != -1)
	{
		const auto [LastMoveOldSquareMin, LastMoveOldSquareMax] = GetSquare(Piece::SquareToRankFile(LastMove.OldSquare));
		const auto [LastMoveNewSquareMin, LastMoveNewSquareMax] = GetSquare(Piece::SquareToRankFile(LastMove.NewSquare));

		DrawList->AddRectFilled(LastMoveOldSquareMin, LastMoveOldSquareMax, COL_SELECTED_SQUARE);
		DrawList->AddRectFilled(LastMoveNewSquareMin, LastMoveNewSquareMax, COL_SELECTED_SQUARE);
	}
}

void ChessEngine::DrawAllowedMoves() const
{
	if (!SelectedPiece)
	{
		return;
	}

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	IterateSquares([this, DrawList](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int Square = Piece::RankFileToSquare({ Rank, File });

			decltype(Pieces)::const_iterator OutCaputedPiece = Pieces.end();

			if (IsAllowedMove(Square, &OutCaputedPiece, nullptr))
			{
				float Diameter = (Max - Min).x;

				if (OutCaputedPiece != Pieces.end())
				{
					DrawList->AddCircle((Min + Max) / 2.f, Diameter / 2.f - Diameter / 20.f, COL_ALLOWEDMOVE, 0, Diameter / 10.f);
				}
				else
				{
					DrawList->AddCircleFilled((Min + Max) / 2.f, Diameter / 6.f, COL_ALLOWEDMOVE);
				}
			}

			return true;
		});
}

void ChessEngine::DrawPieces() const
{
	for (const auto& Piece : Pieces)
	{
		bool bShouldDrawPiece = Piece.get() != SelectedPiece;

		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			bShouldDrawPiece = true;
		}

		if (bShouldDrawPiece)
		{
			DrawPiece(*Piece);
		}
	}
}

void ChessEngine::Update()
{
	HandleInput();
}

void ChessEngine::Draw() const
{
#ifdef _DEBUG
	MarkedSquares.clear();
#endif

	DrawGrid();
	DrawMoveInfo();
	DrawSelectedPiece();
	DrawAllowedMoves();
	DrawPieces();

#ifdef _DEBUG
	DrawMarkedSquares();
#endif

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->ChannelsMerge();
}
