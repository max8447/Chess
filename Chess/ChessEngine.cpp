#include "ChessEngine.h"

#include "ImageHelper.h"

#define PADDING (ImVec2(50.f, 50.f))

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

	// WHITE

	Pieces.push_back(std::make_unique<Piece>(0, Rook, White));
	Pieces.push_back(std::make_unique<Piece>(1, Knight, White));
	Pieces.push_back(std::make_unique<Piece>(2, Bishop, White));
	Pieces.push_back(std::make_unique<Piece>(3, Queen, White));
	Pieces.push_back(std::make_unique<Piece>(4, King, White));
	Pieces.push_back(std::make_unique<Piece>(5, Bishop, White));
	Pieces.push_back(std::make_unique<Piece>(6, Knight, White));
	Pieces.push_back(std::make_unique<Piece>(7, Rook, White));

	for (int Square = 8; Square < 16; Square++)
	{
		Pieces.push_back(std::make_unique<Piece>(Square, Pawn, White));
	}

	// BLACK

	Pieces.push_back(std::make_unique<Piece>(56, Rook, Black));
	Pieces.push_back(std::make_unique<Piece>(57, Knight, Black));
	Pieces.push_back(std::make_unique<Piece>(58, Bishop, Black));
	Pieces.push_back(std::make_unique<Piece>(59, Queen, Black));
	Pieces.push_back(std::make_unique<Piece>(60, King, Black));
	Pieces.push_back(std::make_unique<Piece>(61, Bishop, Black));
	Pieces.push_back(std::make_unique<Piece>(62, Knight, Black));
	Pieces.push_back(std::make_unique<Piece>(63, Rook, Black));

	for (int Square = 48; Square < 56; Square++)
	{
		Pieces.push_back(std::make_unique<Piece>(Square, Pawn, Black));
	}
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

bool ChessEngine::IsAllowedMove(int NewSquare, bool bRemoveCapturedPieces)
{
	IM_ASSERT(NewSquare >= 0 && NewSquare <= 63);

	auto RotateFunction = SelectedPiece->Color == White ? Piece::RotateCCW : Piece::RotateCW;

	auto [OldRank, OldFile] = RotateFunction(Piece::SquareToRankFile(SelectedPiece->Square));
	auto [NewRank, NewFile] = RotateFunction(Piece::SquareToRankFile(NewSquare));

	int DeltaRank = NewRank - OldRank;
	int DeltaFile = NewFile - OldFile;

	int AbsDeltaRank = abs(DeltaRank);
	int AbsDeltaFile = abs(DeltaFile);

	bool bIsMoveAllowed = SelectedPiece->IsAllowedMove(NewSquare);

	// TODO: disallow jumping over pieces with bishop, rook and queen but allow it with knight

	decltype(Pieces)::const_iterator CapturedPieceIt = GetPieceIt(NewSquare);

	bool bHasPiece = CapturedPieceIt != Pieces.end();

	if (bHasPiece)
	{
		if ((*CapturedPieceIt)->Color == SelectedPiece->Color)
		{
			bHasPiece = false;
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
			else // en passant capture
			{
				auto EnpassantRotateFunction = SelectedPiece->Color == White ? Piece::RotateCW : Piece::RotateCCW;

				const auto [EnpassantRank, EnpassantFile] = EnpassantRotateFunction({OldRank, OldFile + DeltaFile});

				int EnpassantSquare = Piece::RankFileToSquare(EnpassantRank, EnpassantFile);

				if (EnpassantSquare == LastEnpassantSquare)
				{
					decltype(Pieces)::const_iterator EnpassantPieceIt = GetPieceIt(EnpassantSquare);

					bHasPiece = EnpassantPieceIt != Pieces.end();

					if (bHasPiece)
					{
						if ((*EnpassantPieceIt)->Color != SelectedPiece->Color &&
							(*EnpassantPieceIt)->Type == Pawn)
						{
							bIsMoveAllowed = true;
							CapturedPieceIt = EnpassantPieceIt;
						}
					}
				}
			}
		}
		else if (AbsDeltaRank == 2 && AbsDeltaFile == 0 && OldRank == 1 && !bHasPiece) // pawn's first move
		{
			bIsMoveAllowed = true;

			LastEnpassantSquare = NewSquare;
			bAllowEnpassant = true;
		}
		else if (bHasPiece) // pawn getting blocked
		{
			bIsMoveAllowed = false;
		}
	}

	if (!bAllowEnpassant)
	{
		LastEnpassantSquare = -1;
	}

	if (bRemoveCapturedPieces && bHasPiece && bIsMoveAllowed)
	{
		Pieces.erase(CapturedPieceIt); // this is safe because we erase before we ever draw any pieces
	}

	return bIsMoveAllowed;
}

void ChessEngine::FinishMove()
{
	CurrentMove = (PieceColor)(1 - CurrentMove);
}

void ChessEngine::SnapSelectedPiece(const ImVec2& Pos)
{
	if (!SelectedPiece)
	{
		return;
	}

	int NewSquare = -1;

	IterateSquares([Pos, &NewSquare](const ImVec2& Min, const ImVec2& Max, int Rank, int File)
		{
			if (Pos.x >= Min.x && Pos.x <= Max.x &&
				Pos.y >= Min.y && Pos.y <= Max.y)
			{
				NewSquare = Piece::RankFileToSquare(Rank, File);
				return false;
			}

			return true;
		});

	IM_ASSERT(NewSquare >= 0 && NewSquare <= 63);

	if (SelectedPiece->Square == NewSquare)
	{
		return; // no move happened
	}

	bool bIsMoveAllowed = IsAllowedMove(NewSquare, true);

	if (bIsMoveAllowed)
	{
		SelectedPiece->Square = NewSquare;

		FinishMove();
	}
}

void ChessEngine::UpdateSelectedPiece()
{
	const ImVec2 MousePos = ImGui::GetMousePos();

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
		SelectedPiece)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		DrawList->ChannelsSplit(2);
		DrawList->ChannelsSetCurrent(1);

		DrawPiece(MousePos - SelectedPieceMouseOffset, SelectedPiece->Type, SelectedPiece->Color);

		DrawList->ChannelsSetCurrent(0);
	}
	else
	{
		SnapSelectedPiece(MousePos);
		SelectedPieceMouseOffset = {};
		SelectedPiece = nullptr;
	}
}

void ChessEngine::SelectPiece()
{
	const ImVec2 MousePos = ImGui::GetMousePos();

	bool bFoundPiece = false;

	IterateSquares([MousePos, &bFoundPiece, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File)
		{
			if (MousePos.x >= Min.x && MousePos.x <= Max.x &&
				MousePos.y >= Min.y && MousePos.y <= Max.y)
			{
				if (SelectedPiece = GetPiece(Rank, File))
				{
					SelectedPieceMouseOffset = MousePos - Min;
				}

				bFoundPiece = SelectedPiece != nullptr;
				return false;
			}

			return true;
		});

	if (!bFoundPiece)
	{
		SelectedPiece = nullptr;
	}
	else
	{
		if (SelectedPiece->Color != CurrentMove)
		{
			SelectedPiece = nullptr;
		}
	}
}

void ChessEngine::HandleInput()
{
	if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		if (!SelectedPiece)
		{
			SelectPiece();
		}
	}
	else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		SelectedPiece = nullptr;
	}
}

Piece* ChessEngine::GetPiece(int Rank, int File) const
{
	int Square = Piece::RankFileToSquare(Rank, File);

	if (Square < 0 || Square > 63)
	{
		return nullptr;
	}

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

void ChessEngine::DrawGrid() const
{
#define COL_BG IM_COL32(50, 50, 50, 255)
#define COL_BLACK IM_COL32(118, 150, 86, 255)
#define COL_WHITE IM_COL32(231, 231, 204, 255)

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 Min = Center - Radius;
	const ImVec2 Max = Center + Radius;

	DrawList->AddRectFilled(Min, Max, COL_BG);

	IterateSquares([DrawList, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File)
		{
			int ColorOffset = Rank % 2;
			int ColorSwitch = (File + ColorOffset) % 2;

			ImU32 Color = ColorSwitch ? COL_BLACK : COL_WHITE;

#ifdef _DEBUG
			int Square = Piece::RankFileToSquare(Rank, File);

			if (std::find(SquaresToMark.begin(), SquaresToMark.end(), Square) != SquaresToMark.end())
			{
				Color = IM_COL32(255, 0, 0, 255);
			}
#endif

			DrawList->AddRectFilled(Min, Max, Color);

			return true;
		});
}

void ChessEngine::DrawCurrentMove() const
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareRadius = (Radius - PADDING) / 4;

	int Rank = -2;
	int File = 0;

	ImVec2 x = Center + (Rank - 4) * SquareRadius;
	ImVec2 y = Center + (File - 4) * SquareRadius;

	ImVec2 SquareMin = ImVec2(x.x, y.y);
	ImVec2 SquareMax = SquareMin + SquareRadius;

	ImU32 CurrentColor = CurrentMove == White ? IM_COL32_WHITE : IM_COL32_BLACK;

	DrawList->AddRectFilled(SquareMin - PADDING, SquareMax + PADDING, COL_BG);
	DrawList->AddRectFilled(SquareMin, SquareMax, CurrentColor);
}

void ChessEngine::DrawPieces() const
{
	for (const auto& Piece : Pieces)
	{
		if (Piece.get() != SelectedPiece)
		{
			DrawPiece(*Piece);
		}
	}
}

void ChessEngine::Update()
{
	HandleInput();
	UpdateSelectedPiece();
}

void ChessEngine::Draw() const
{
	DrawGrid();
	DrawCurrentMove();
	DrawPieces();

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->ChannelsMerge();
}
