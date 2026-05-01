#include "ChessEngine.h"

#include "ImageHelper.h"

#define PADDING (ImVec2(50.f, 50.f))

#define COL_BG IM_COL32(50, 50, 50, 255)
#define COL_BLACK IM_COL32(118, 150, 86, 255)
#define COL_WHITE IM_COL32(231, 231, 204, 255)
#define COL_ALLOWEDMOVE IM_COL32(0, 0, 0, 50)
#define COL_MARKED_SQUARE IM_COL32(255, 0, 0, 127)
#define COL_SELECTED_SQUARE IM_COL32(255, 255, 0, 127)
#define COL_ENPASSANT_SQUARE IM_COL32(0, 255, 0, 127)
#define COL_PAWNPROMOTION_BG IM_COL32_WHITE
#define COL_ENDSCREEN_OVERLAY IM_COL32(0, 0, 0, 127)

ChessEngine::ChessEngine(const char* FENString)
	: SelectedPiece(nullptr), FENString(FENString)
{
	int ImageWidth = 0;
	int ImageHeight = 0;

	bool bSuccess = LoadTextureFromFile("../Assets/Chess_Pieces_Sprite.png", &PiecesImageTexture, &ImageWidth, &ImageHeight);

	if (!bSuccess)
	{
		bSuccess = LoadTextureFromFile("../../Assets/Chess_Pieces_Sprite.png", &PiecesImageTexture, &ImageWidth, &ImageHeight);
	}

	ASSERT(bSuccess && "Failed to load pieces texture!");

	LoadFENPosition(FENString);
}

void ChessEngine::LoadFENPosition(const char* FENString)
{
	Pieces.clear();

	SelectedPieceMouseOffset = { -FLT_MAX, FLT_MAX };
	SelectedPiece = nullptr;

	AvailableMoves.clear();
	NumPossibleMoves = -1;

	CurrentMove = White;
	CastlingRights = {};
	EnpassantSquare = -1;
	PawnPromotionSquare = -1;
	HalfMoveClock = 0;
	FullMoveCounter = 1;
	LastMove = {};

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
				CastlingRights.KingSide[0].Invalidate();
				CastlingRights.KingSide[1].Invalidate();

				CastlingRights.QueenSide[0].Invalidate();
				CastlingRights.QueenSide[1].Invalidate();

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
				CurrentFile += c - '0';
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
					ASSERT(false);
					break;
				}

				int Square = Piece::RankFileToSquare(CurrentRank, CurrentFile++);

				Pieces.push_back(std::make_unique<Piece>(Square, Type, Color));
			}
			else if (!bSetupCurrentMove)
			{
				ASSERT(c == 'w' || c == 'b');

				CurrentMove = c == 'w' ? White : Black;

				bSetupCurrentMove = true;
			}
			else if (!bSetupCastling)
			{
				bool KingSideEnabled[2] = { false, false };
				bool QueenSideEnabled[2] = { false, false };

				FENString--;

				while (!isspace(c = *FENString++))
				{
					PieceColor Color = isupper(c) ? White : Black;

					switch (tolower(c))
					{
					case 'k':
						KingSideEnabled[Color] = true;
						break;
					case 'q':
						QueenSideEnabled[Color] = true;
						break;
					default:
						ASSERT(false);
						break;
					}
				}

				for (int i = 0; i < 2; i++)
				{
					if (!KingSideEnabled[i])
					{
						CastlingRights.KingSide[i].Invalidate();
					}
					if (!QueenSideEnabled[i])
					{
						CastlingRights.QueenSide[i].Invalidate();
					}
				}

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

	ASSERT(bSetupPlacement && bSetupCurrentMove && bSetupCastling && bSetupEnpassant && bSetupHalfMoveClock && bSetupFullMoveCounter);
}

void ChessEngine::IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const
{
	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const ImVec2 SquareSize = GetSquareSize();

	for (int Rank = 0; Rank < 8; Rank++)
	{
		for (int File = 0; File < 8; File++)
		{
			ImVec2 x = Center + (Rank - 4.f) * SquareSize;
			ImVec2 y = Center + (File - 4.f) * SquareSize;

			ImVec2 SquareMin = ImVec2(x.x, y.y);
			ImVec2 SquareMax = SquareMin + SquareSize;

			if (!Predicate(SquareMin, SquareMax, Rank, File))
			{
				return;
			}
		}
	}
}

void ChessEngine::GetAllAvailableMoves(PieceColor Color, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(OutAvailableMoves);

	constexpr size_t MaxAvailableMoves = 218; // https://www.chessprogramming.org/Chess_Position

	OutAvailableMoves->reserve(MaxAvailableMoves);

	for (const auto& Piece : Pieces)
	{
		if (Piece->Color == Color && Piece->Square != -1)
		{
			GetAvailableMoves(Piece.get(), OutAvailableMoves, bAllowPseudolegalMoves);
		}
	}
}

void ChessEngine::GetAvailableMoves(Piece* TargetPiece, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(TargetPiece && OutAvailableMoves);
	
	constexpr size_t MaxAvailableMoves = 27; // a queen in the middle of the board

	OutAvailableMoves->reserve(MaxAvailableMoves);

	IterateSquares([&OutAvailableMoves, TargetPiece, bAllowPseudolegalMoves, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int Square = Piece::RankFileToSquare(Rank, File);

			SpecialMove OutSpecialMove;
			if (IsAllowedMove(TargetPiece, Square, bAllowPseudolegalMoves, &OutSpecialMove) && OutSpecialMove.IsAllowed())
			{
				OutAvailableMoves->push_back(OutSpecialMove);
			}

			return true;
		});
}

void ChessEngine::CalculatePossibleMoves()
{
	std::vector<SpecialMove> PossibleMoves;
	bool bAllowPseudolegal = false;

	GetAllAvailableMoves(CurrentMove, &PossibleMoves, bAllowPseudolegal);

	NumPossibleMoves = PossibleMoves.size();
}

bool ChessEngine::IsAllowedMove(Piece* MovingPiece, int NewSquare, bool bAllowPseudolegal, SpecialMove* OutSpecialMove) const
{
#ifdef _DEBUG
	using namespace std::chrono;

	steady_clock::time_point IsAllowedMoveStart = high_resolution_clock::now();
#endif

	ASSERT(MovingPiece && OutSpecialMove, false);
	ASSERT(NewSquare >= 0 && NewSquare <= 63, false);

	if (MovingPiece->Square == NewSquare)
	{
		return false; // no move happened
	}

	OutSpecialMove->Invalidate();
	OutSpecialMove->Move = { MovingPiece->Square, NewSquare};
	OutSpecialMove->MovedPiece = MovingPiece;

	const auto [OldRank, OldFile] = MovingPiece->GetRankFile();
	const auto [NewRank, NewFile] = MovingPiece->GetRankFile(NewSquare);
	
	const int DeltaRank = NewRank - OldRank;
	const int DeltaFile = NewFile - OldFile;
	
	const int AbsDeltaRank = abs(DeltaRank);
	const int AbsDeltaFile = abs(DeltaFile);
	
	const int DeltaRankDir = AbsDeltaRank == 0 ? 0 : DeltaRank / AbsDeltaRank;
	const int DeltaFileDir = AbsDeltaFile == 0 ? 0 : DeltaFile / AbsDeltaFile;

	bool bIsMoveAllowed = MovingPiece->IsAllowedMove(NewSquare);

	Piece* CapturedPiece = GetPiece(NewSquare);

	bool bHasPieceOfSameColor = false;

	if (CapturedPiece)
	{
		if (CapturedPiece->Color == MovingPiece->Color)
		{
			CapturedPiece = nullptr;
			bHasPieceOfSameColor = true;

			bIsMoveAllowed = false;
		}
		else
		{
			OutSpecialMove->Type = Capture;
			OutSpecialMove->OtherPieceMove = { NewSquare, -1 };
			OutSpecialMove->CapturedPiece = CapturedPiece;
		}
	}

	bool bAllowEnpassant = false;

	if (MovingPiece->Type == Pawn)
	{
		if (DeltaRank == 1 && AbsDeltaFile == 1)
		{
			if (CapturedPiece) // normal pawn capture
			{
				bIsMoveAllowed = true;
			}
			else if (EnpassantSquare != -1 && DeltaRank > 0) // en passant capture
			{
				int EnpassantSquareToTest = MovingPiece->GetSquare(OldRank, OldFile + DeltaFile);

				if (EnpassantSquareToTest == EnpassantSquare)
				{
					CapturedPiece = GetPiece(EnpassantSquare);

					if (CapturedPiece)
					{
						if (CapturedPiece->Color != MovingPiece->Color &&
							CapturedPiece->Type == Pawn)
						{
							bIsMoveAllowed = true;

							OutSpecialMove->Type = Capture;
							OutSpecialMove->OtherPieceMove = { EnpassantSquare, -1 };
							OutSpecialMove->CapturedPiece = CapturedPiece;
						}
					}
				}
			}
		}
		else if (AbsDeltaRank == 2 && AbsDeltaFile == 0 && OldRank == 1 && !CapturedPiece && !bHasPieceOfSameColor) // pawn's first move
		{
			int SquareInFront = MovingPiece->GetSquare(OldRank + 1, OldFile);

			Piece* PieceInFront = GetPiece(SquareInFront);

			bIsMoveAllowed = PieceInFront == nullptr;

			if (bIsMoveAllowed)
			{
				OutSpecialMove->Type = PossibleEnpassant;
				OutSpecialMove->OtherPieceMove = OutSpecialMove->Move;

				bAllowEnpassant = true;
			}
		}
		else if (CapturedPiece) // pawn getting blocked
		{
			bIsMoveAllowed = false;
		}
	}
	else if (MovingPiece->Type == Bishop || MovingPiece->Type == Rook || MovingPiece->Type == Queen)
	{
		bool bRayHitPiece = false;

		auto CheckSquare = [&bIsMoveAllowed, &bRayHitPiece, MovingPiece, this](int Square) -> bool
			{
				if (bRayHitPiece)
				{
					bIsMoveAllowed = false;
					return true;
				}

				if (Piece* SkippedPiece = GetPiece(Square))
				{
					if (SkippedPiece->Color == MovingPiece->Color)
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

				int CurrentSquare = MovingPiece->GetSquare(CurrentRank, CurrentFile);

				if (CheckSquare(CurrentSquare))
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

				int CurrentSquare = MovingPiece->GetSquare(CurrentRank, NewFile);

				if (CheckSquare(CurrentSquare))
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

				int CurrentSquare = MovingPiece->GetSquare(NewRank, CurrentFile);

				if (CheckSquare(CurrentSquare))
				{
					break;
				}

				CurrentFile += DeltaFileDir;
			}
		}
	}
	else if (MovingPiece->Type == King)
	{
		if (!bIsMoveAllowed) // might be trying to castle?
		{
			if (NewFile + DeltaFileDir >= 0 && NewFile + DeltaFileDir <= 7 &&
				OldFile + DeltaFileDir >= 0 && OldFile + DeltaFileDir <= 7)
			{
				int CastlingRookSquare = MovingPiece->GetSquare(OldRank, NewFile + DeltaFileDir);
				Piece* CastlingRook = GetPiece(CastlingRookSquare);

				if (!CastlingRook)
				{
					if (NewFile + 2 * DeltaFileDir >= 0 && NewFile + 2 * DeltaFileDir <= 7)
					{
						CastlingRookSquare = MovingPiece->GetSquare(OldRank, NewFile + 2 * DeltaFileDir);
						CastlingRook = GetPiece(CastlingRookSquare);
					}
				}

				if (CastlingRook && CastlingRook->Type == Rook && CastlingRook->Color == MovingPiece->Color)
				{
					int SquareBetween = MovingPiece->GetSquare(OldRank, OldFile + DeltaFileDir);
					Piece* PieceBetween = GetPiece(SquareBetween);

					if (!CapturedPiece && !PieceBetween)
					{
						auto AllowCastle = [&bIsMoveAllowed, &OutSpecialMove, CastlingRookSquare, SquareBetween, CastlingRook]()
							{
								bIsMoveAllowed = true;

								OutSpecialMove->Type = Castle;
								OutSpecialMove->OtherPieceMove = {
										CastlingRookSquare,	// old rook square
										SquareBetween,		// new rook square
								};
								OutSpecialMove->CastledRook = CastlingRook;
							};

						SimpleMove KingSideCastle = CastlingRights.KingSide[MovingPiece->Color].Move;
						SimpleMove QueenSideCastle = CastlingRights.QueenSide[MovingPiece->Color].Move;

						if (KingSideCastle.IsAllowed() && NewSquare == KingSideCastle.NewSquare)
						{
							AllowCastle();
						}
						else if (QueenSideCastle.IsAllowed() && NewSquare == QueenSideCastle.NewSquare)
						{
							int SecondSquareBetween = MovingPiece->GetSquare(OldRank, NewFile + DeltaFileDir);
							Piece* SecondPieceBetween = GetPiece(SecondSquareBetween);

							if (!SecondPieceBetween)
							{
								AllowCastle();
							}
						}
					}
				}
			}
		}
	}

	if (bIsMoveAllowed)
	{
		if (!bAllowPseudolegal)
		{
			// don't allow the move if it leaves us in check after

			MakeMove(*OutSpecialMove);

			if (OutSpecialMove->Type == Castle && OutSpecialMove->OtherPieceMove.IsAllowed())
			{
				// a castling king may not pass through a piece that is under attack

				if (OutSpecialMove->CastledRook && IsAttacked(OutSpecialMove->CastledRook))
				{
					bIsMoveAllowed = false;
				}
			}

			if (bIsMoveAllowed) // might've been disallowed by castling check above
			{
				if (IsInCheck(MovingPiece->Color))
				{
					bIsMoveAllowed = false;
				}
			}

			UnMakeMove(*OutSpecialMove);
		}
	}

	if (!bIsMoveAllowed)
	{
		OutSpecialMove->Invalidate();
	}

#ifdef _DEBUG
	steady_clock::time_point IsAllowedMoveEnd = high_resolution_clock::now();
	nanoseconds IsAllowedMoveDuration = duration_cast<nanoseconds>(IsAllowedMoveEnd - IsAllowedMoveStart);

	printf(__FUNCTION__" took %lld ns\n", IsAllowedMoveDuration.count());

	if (IsAllowedMoveDuration > 4us)
	{
		auto [LongRank, LongFile] = MovingPiece->GetRankFile(NewSquare);
		LongFile = 7 - LongFile;
		printf("%s took long\n", Piece::RankFileToAlgebraic({ LongRank, LongFile }).data());
	}
#endif

	return bIsMoveAllowed;
}

void ChessEngine::MakeMove(const SpecialMove& Move) const
{
	ASSERT(Move.MovedPiece && Move.Move.IsAllowed());

	Move.MovedPiece->Square = Move.Move.NewSquare;

	Piece* OtherPiece = nullptr;

	if (Move.OtherPieceMove.IsAllowed())
	{
		if (Move.Type == Castle)
		{
			OtherPiece = Move.CastledRook;
		}
		else if (Move.Type == Capture)
		{
			OtherPiece = Move.CapturedPiece;
		}
	}

	if (OtherPiece)
	{
		OtherPiece->Square = Move.OtherPieceMove.NewSquare;
	}
}

void ChessEngine::UnMakeMove(const SpecialMove& Move) const
{
	ASSERT(Move.MovedPiece && Move.Move.IsAllowed());

	Move.MovedPiece->Square = Move.Move.OldSquare;

	Piece* OtherPiece = nullptr;

	if (Move.OtherPieceMove.IsAllowed())
	{
		if (Move.Type == Castle)
		{
			OtherPiece = Move.CastledRook;
		}
		else if (Move.Type == Capture)
		{
			OtherPiece = Move.CapturedPiece;
		}
	}

	if (OtherPiece)
	{
		OtherPiece->Square = Move.OtherPieceMove.OldSquare;
	}
}

bool ChessEngine::IsInCheck(PieceColor Color) const
{
	Piece* King = GetFirstPiece(PieceType::King, Color);

	if (!King)
	{
		return false;
	}

	return IsAttacked(King);
}

bool ChessEngine::IsAttacked(Piece* AttackedPiece) const
{
	ASSERT(AttackedPiece, false);

	const auto [Rank, File] = AttackedPiece->GetRankFile();

	enum CheckType
	{
		Ranks,
		Files,
		DiagonalsPos,
		DiagonalsNeg
	};

	auto DoCheck = [AttackedPiece, Rank, File, this](CheckType CheckType, int Dir) -> bool
		{
			for (int i = 1; i < 8; i++)
			{
				const int CurrentRank = Rank + (i * Dir * (CheckType == DiagonalsNeg ? -1 : 1)	* (CheckType != Files));
				const int CurrentFile = File + (i * Dir											* (CheckType != Ranks));

				if (CurrentRank < 0 || CurrentRank > 7 ||
					CurrentFile < 0 || CurrentFile > 7)
				{
					break;
				}

				const int CurrentSquare = AttackedPiece->GetSquare(CurrentRank, CurrentFile);

				Piece* HitPiece = GetPiece(CurrentSquare);

				if (HitPiece)
				{
					if (HitPiece->Color == AttackedPiece->Color)
					{
						return false;
					}
					else
					{
						const int DeltaRank = CurrentRank - Rank;
						const int DeltaFile = CurrentFile - File;

						const int AbsDeltaRank = abs(CurrentRank - Rank);
						const int AbsDeltaFile = abs(CurrentFile - File);

						const int Distance = max(AbsDeltaRank, AbsDeltaFile);

						bool bIsPieceAttacking = false;

						switch (HitPiece->Type)
						{
						case King:
							bIsPieceAttacking = Distance == 1;
							break;
						case Queen:
							bIsPieceAttacking = true;
							break;
						case Bishop:
							bIsPieceAttacking = CheckType == DiagonalsPos || CheckType == DiagonalsNeg;
							break;
						case Knight:
							bIsPieceAttacking = false; // we don't check for knight attacks in this function
							break;
						case Rook:
							bIsPieceAttacking = CheckType == Ranks || CheckType == Files;
							break;
						case Pawn:
							bIsPieceAttacking = DeltaRank == 1 && AbsDeltaFile == 1 && Distance == 1 && (CheckType == DiagonalsPos || CheckType == DiagonalsNeg);
							break;
						default:
							break;
						}

						return bIsPieceAttacking;
					}
				}
			}

			return false;
		};

	for (int i = 0; i < 2; i++)
	{
		const int Dir = i == 0 ? 1 : -1;

		if (DoCheck(Ranks, Dir) ||
			DoCheck(Files, Dir) ||
			DoCheck(DiagonalsPos, Dir) ||
			DoCheck(DiagonalsNeg, Dir))
		{
			return true;
		}
	}

	// knight-attack implementation

	for (int i = 0; i < 2; i++)
	{
		int AbsDeltaRank = (i == 0) + 1;
		int AbsDeltaFile = (i == 1) + 1;

		for (int j = 0; j < 4; j++)
		{
			int RankDir = (j & 1) ? 1 : -1;
			int FileDir = (j & 2) ? 1 : -1;

			const int DeltaRank = RankDir * AbsDeltaRank;
			const int DeltaFile = FileDir * AbsDeltaFile;

			const int CurrentRank = Rank + DeltaRank;
			const int CurrentFile = File + DeltaFile;

			const int CurrentSquare = AttackedPiece->GetSquare(CurrentRank, CurrentFile);

			Piece* HitPiece = GetPiece(CurrentSquare);

			if (HitPiece)
			{
				if (HitPiece->Color != AttackedPiece->Color && HitPiece->Type == Knight)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void ChessEngine::RemoveCastlingRights(Piece* MovedPiece, int NewSquare)
{
	ASSERT(MovedPiece);

	PieceColor Color = MovedPiece->Color;

	if (CastlingRights.KingSide[Color].IsAllowed() ||
		CastlingRights.QueenSide[Color].IsAllowed())
	{
		if (MovedPiece->Type == King)
		{
			CastlingRights.KingSide[Color].Invalidate();
			CastlingRights.QueenSide[Color].Invalidate();
		}
		else if (MovedPiece->Type == Rook)
		{
			int DefaultKingSquare = CastlingRights.KingSide[Color].Move.OldSquare;

			if (DefaultKingSquare < 0 || DefaultKingSquare > 63)
			{
				return;
			}

			const auto [OldRank, OldFile] = Piece::SquareToRankFile(NewSquare);
			const auto [DefaultKingRank, DefaultKingFile] = Piece::SquareToRankFile(DefaultKingSquare);

			if (OldFile - DefaultKingFile > 0) // king side
			{
				CastlingRights.KingSide[Color].Invalidate();
			}
			else // queen side
			{
				CastlingRights.QueenSide[Color].Invalidate();
			}
		}
	}
}

void ChessEngine::FinishMove(Piece* MovingPiece, const SpecialMove& Move)
{
	ASSERT(MovingPiece);

	if (CurrentMove == Black)
	{
		FullMoveCounter++;
	}

	if (MovingPiece->Type == Pawn)
	{
		HalfMoveClock = 0;

		const auto [Rank, File] = MovingPiece->GetRankFile(Move.Move.NewSquare);

		if (Rank == 7)
		{
			PawnPromotionSquare = Move.Move.NewSquare;
		}
	}

	if (Move.Type == PossibleEnpassant)
	{
		EnpassantSquare = Move.OtherPieceMove.NewSquare;
	}
	if (Move.Type == Castle && Move.OtherPieceMove.IsAllowed())
	{
		ASSERT(Move.CastledRook);

		Move.CastledRook->Square = Move.OtherPieceMove.NewSquare;
	}

	LastMove = Move;
	MovingPiece->Square = Move.Move.NewSquare;

	CurrentMove = (PieceColor)(1 - CurrentMove);

	CalculatePossibleMoves();
}

void ChessEngine::TryMoveTo(Piece* MovingPiece, int NewSquare)
{
	ASSERT(MovingPiece);
	
	bool bAllowPseudolegal = false;
	SpecialMove OutSpecialMove;

	bool bIsMoveAllowed = IsAllowedMove(MovingPiece, NewSquare, bAllowPseudolegal, &OutSpecialMove);

	if (bIsMoveAllowed && OutSpecialMove.IsAllowed())
	{
		HalfMoveClock++; // this *might* get set to zero in CapturePiece() or FinishMove()

		if (MovingPiece->Type == King || MovingPiece->Type == Rook)
		{
			RemoveCastlingRights(MovingPiece, NewSquare);
		}

		
		if (OutSpecialMove.Type == Capture)
		{
			Piece* CapturedPiece = OutSpecialMove.CapturedPiece;

			if (CapturedPiece)
			{
				if (CapturedPiece->Type == King || CapturedPiece->Type == Rook) // can never be king but you never know
				{
					RemoveCastlingRights(CapturedPiece, CapturedPiece->Square);
				}

				CapturePiece(CapturedPiece);
			}
		}

		FinishMove(MovingPiece, OutSpecialMove);
	}
}

void ChessEngine::CapturePiece(Piece* CapturedPiece)
{
	ASSERT(CapturedPiece);

	HalfMoveClock = 0;

	CapturedPiece->Square = -1;
}

void ChessEngine::SnapSelectedPiece(const ImVec2& Pos)
{
	if (!SelectedPiece)
	{
		return;
	}

	int NewSquare = Piece::RotateCW(GetSquare(Pos));

	if (NewSquare < 0 || NewSquare > 63)
	{
		return;
	}

	TryMoveTo(SelectedPiece, NewSquare);
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
			if (Pos >= Min && Pos <= Max)
			{
				if (SelectedPiece = GetPiece(Piece::RankFileToSquare(Piece::RotateCW({ Rank, File }))))
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
		if (SelectedPiece)
		{
			if (SelectedPiece->Color != CurrentMove)
			{
				SelectedPiece = nullptr;
			}
			else // generate all legal moves once to be used later on
			{
				bool bAllowPseudolegalMoves = false;

				AvailableMoves.clear();
				GetAvailableMoves(SelectedPiece, &AvailableMoves, bAllowPseudolegalMoves);
			}
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

	if (NumPossibleMoves > 0)
	{
		if (PawnPromotionSquare == -1)
		{
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
		else
		{
			Piece* PromotingPawn = GetPiece(PawnPromotionSquare);

			ASSERT(PromotingPawn);

			PieceType OldPieceType = PromotingPawn->Type;

			const auto [Rank, File] = PromotingPawn->GetRankFile();

			int OldSquare = Piece::RankFileToSquare(Rank - 1, File);

			const ImVec2 SquareSize = GetSquareSize();
			auto [PromotionSquareMin, PromotionSquareMax] = GetSquare(Piece::RotateCCW(PawnPromotionSquare));

			if (PromotingPawn->Color == Black)
			{
				PromotionSquareMin.y -= 3.f * SquareSize.y;
			}

			if (bLeftMouseClicked)
			{
				bool bPromoted = false;

#define CHECK_FOR(PieceType, Distance) \
const ImVec2 PieceType##Pos = ImVec2(PromotionSquareMin.x, PromotionSquareMin.y + (float)Distance * SquareSize.y); \
if (MousePos >= PieceType##Pos && MousePos <= PieceType##Pos + SquareSize) { PromotingPawn->Type = PieceType; bPromoted = true; }

				CHECK_FOR(Queen, 0);
				CHECK_FOR(Knight, 1);
				CHECK_FOR(Rook, 2);
				CHECK_FOR(Bishop, 3);

#undef CHECK_FOR

				if (bPromoted)
				{
					PawnPromotionSquare = -1;
					LastMove.Invalidate();

					LastMove.Type = PawnPromotion;
					LastMove.Move = { OldSquare, PromotingPawn->Square };
					LastMove.OldPieceType = OldPieceType;
				}
			}
		}
	}
	else if (NumPossibleMoves == 0) // can also be -1 to signal not initialized
	{
		if (bLeftMouseClicked)
		{
			LoadFENPosition(FENString);
		}
	}
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

Piece* ChessEngine::GetFirstPiece(PieceType Type, PieceColor Color) const
{
	for (const auto& Piece : Pieces)
	{
		if (Piece->Type == Type && Piece->Color == Color)
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
	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const ImVec2 SquareSize = GetSquareSize();

	const auto [Rank, File] = RankFile;

	ImVec2 X = Center + (Rank - 4.f) * SquareSize;
	ImVec2 Y = Center + (File - 4.f) * SquareSize;

	ImVec2 Min = ImVec2(X.x, Y.y);
	ImVec2 Max = Min + SquareSize;

	return { Min, Max };
}

std::pair<ImVec2, ImVec2> ChessEngine::GetSquare(int Square) const
{
	return GetSquare(Piece::SquareToRankFile(Square));
}

ImVec2 ChessEngine::GetSquareSize() const
{
	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const float SmallestAxis = min(Center.x, Center.y);

	const ImVec2 Radius = ImVec2(SmallestAxis, SmallestAxis) - PADDING;
	const ImVec2 SquareSize = (Radius - PADDING) / 4.f;

	return SquareSize;
}

void ChessEngine::DrawPiece(const ImVec2& Pos, const ImVec2& PieceSize, PieceType Type, PieceColor Color) const
{
	ImVec2 p_min = Pos;
	ImVec2 uv_min = ImVec2((float)Type		 / 6.f,	(float)Color	   / 2.f);

	ImVec2 p_max = Pos + PieceSize;
	ImVec2 uv_max = ImVec2((float)(Type + 1) / 6.f, (float)(Color + 1) / 2.f);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->AddImage((ImTextureID)PiecesImageTexture, p_min, p_max, uv_min, uv_max);
}

void ChessEngine::DrawPiece(int Square, PieceType Type, PieceColor Color) const
{
	ASSERT(Square >= 0 && Square <= 63);

	const ImVec2 Size = ImGui::GetWindowSize();

	const ImVec2 Center = Size / 2.f;
	const ImVec2 SquareSize = GetSquareSize();

	const auto [Rank, File] = Piece::RotateCCW(Piece::SquareToRankFile(Square));

	ImVec2 x = Center + (Rank - 4.f) * SquareSize;
	ImVec2 y = Center + (File - 4.f) * SquareSize;

	return DrawPiece(ImVec2(x.x, y.y), SquareSize, Type, Color);
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

			int Square = Piece::RankFileToSquare(Piece::RotateCW({ Rank, File }));

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
			File = 7 - File;

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
		DrawList->ChannelsSetCurrent(1);

		const ImVec2 MousePos = ImGui::GetMousePos();
		DrawPiece(MousePos - SelectedPieceMouseOffset, GetSquareSize(), SelectedPiece->Type, SelectedPiece->Color);

		DrawList->ChannelsSetCurrent(0);
	}

	if (SelectedPiece->Square != LastMove.Move.OldSquare && SelectedPiece->Square != LastMove.Move.NewSquare)
	{
		const auto [Min, Max] = GetSquare(Piece::RotateCCW(Piece::SquareToRankFile(SelectedPiece->Square)));

		DrawList->AddRectFilled(Min, Max, COL_SELECTED_SQUARE);
	}
}

void ChessEngine::DrawMoveInfo() const
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// CurrentMove

	int CurrentMoveRank = -2;
	int CurrentMoveFile = 0;

	const auto [CurrentMoveSquareMin, CurrentMoveSquareMax] = GetSquare(std::pair{ CurrentMoveRank, CurrentMoveFile });

	ImU32 CurrentColor = CurrentMove == White ? IM_COL32_WHITE : IM_COL32_BLACK;

	DrawList->AddRectFilled(CurrentMoveSquareMin - PADDING, CurrentMoveSquareMax + PADDING, COL_BG);
	DrawList->AddRectFilled(CurrentMoveSquareMin, CurrentMoveSquareMax, CurrentColor);

	// CastlingRights

	constexpr bool bDrawCastlingRights = false;

	if constexpr (bDrawCastlingRights)
	{
#ifdef _DEBUG
		if (CastlingRights.KingSide[0].Move.IsAllowed())
		{
			MarkedSquares.push_back(CastlingRights.KingSide[0].Move.NewSquare);
		}

		if (CastlingRights.KingSide[1].Move.IsAllowed())
		{
			MarkedSquares.push_back(CastlingRights.KingSide[1].Move.NewSquare);
		}

		if (CastlingRights.QueenSide[0].Move.IsAllowed())
		{
			MarkedSquares.push_back(CastlingRights.QueenSide[0].Move.NewSquare);
		}

		if (CastlingRights.QueenSide[1].Move.IsAllowed())
		{
			MarkedSquares.push_back(CastlingRights.QueenSide[1].Move.NewSquare);
		}
#endif
	}

	// EnpassantSquare

	constexpr bool bDrawEnpassantSquare = false;

	if (bDrawEnpassantSquare && EnpassantSquare != -1)
	{
		const auto [EnpassantSquareMin, EnpassantSquareMax] = GetSquare(Piece::RotateCCW(Piece::SquareToRankFile(EnpassantSquare)));

		DrawList->AddRectFilled(EnpassantSquareMin, EnpassantSquareMax, COL_ENPASSANT_SQUARE);
	}

	// HalfMoveClock

	ImGui::SetCursorScreenPos(ImVec2(CurrentMoveSquareMin.x - PADDING.x, CurrentMoveSquareMax.y + PADDING.y));
	ImGui::Text("HalfMoveClock: %d", HalfMoveClock);

	// FullMoveCounter

	ImGui::SetCursorScreenPos(ImVec2(CurrentMoveSquareMin.x - PADDING.x, ImGui::GetCursorPosY()));
	ImGui::Text("FullMoveCounter: %d", FullMoveCounter);

	// LastMove

	if (LastMove.Move.IsValid())
	{
		const auto [LastMoveOldSquareMin, LastMoveOldSquareMax] = GetSquare(Piece::RotateCCW(Piece::SquareToRankFile(LastMove.Move.OldSquare)));
		const auto [LastMoveNewSquareMin, LastMoveNewSquareMax] = GetSquare(Piece::RotateCCW(Piece::SquareToRankFile(LastMove.Move.NewSquare)));

		DrawList->AddRectFilled(LastMoveOldSquareMin, LastMoveOldSquareMax, COL_SELECTED_SQUARE);
		DrawList->AddRectFilled(LastMoveNewSquareMin, LastMoveNewSquareMax, COL_SELECTED_SQUARE);
	}
}

void ChessEngine::DrawAvailableMoves() const
{
	if (!SelectedPiece)
	{
		return;
	}

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	for (const SpecialMove& Move : AvailableMoves)
	{
		const auto [MoveMin, MoveMax] = GetSquare(Piece::RotateCCW(Move.Move.NewSquare));

		float Diameter = (MoveMax - MoveMin).x;

		if (Move.Type == Capture)
		{
			DrawList->AddCircle((MoveMin + MoveMax) / 2.f, Diameter / 2.f - Diameter / 20.f, COL_ALLOWEDMOVE, 0, Diameter / 10.f);
		}
		else
		{
			DrawList->AddCircleFilled((MoveMin + MoveMax) / 2.f, Diameter / 6.f, COL_ALLOWEDMOVE);
		}
	}
}

void ChessEngine::DrawPieces() const
{
	const ImVec2 SquareSize = GetSquareSize();
	const ImVec2 PieceSize = SquareSize / 2.f;

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	ImVec2 WhitePos = GetSquare(std::pair{ 8, 0 }).first + PADDING + ImVec2(PADDING.x, 0.f);
	ImVec2 BlackPos = GetSquare(std::pair{ 8, 7 }).first + PADDING + ImVec2(PADDING.x, 0.f);

	for (const auto& Piece : Pieces)
	{
		bool bShouldDrawPiece =
			(Piece.get() != SelectedPiece || !ImGui::IsMouseDragging(ImGuiMouseButton_Left));

		if (bShouldDrawPiece)
		{
			if (Piece->Square == -1) // has been captured
			{
				// TODO: make it draw them better

				ImVec2& Pos = Piece->Color == White ? WhitePos : BlackPos;

				ImU32 BGColor = Piece->Color == White ? COL_WHITE : COL_BLACK;

				DrawList->AddRectFilled(Pos, Pos + PieceSize, BGColor);
				DrawPiece(Pos, PieceSize, Piece->Type, Piece->Color);

				Pos.x += PieceSize.x;
			}
			else
			{
				DrawPiece(*Piece);
			}
		}
	}
}

void ChessEngine::DrawPromotionPopup() const
{
	if (PawnPromotionSquare == -1)
	{
		return;
	}

	Piece* PromotingPawn = GetPiece(PawnPromotionSquare);

	ASSERT(PromotingPawn);

	PieceColor Color = PromotingPawn->Color;

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	const ImVec2 SquareSize = GetSquareSize();
	const ImVec2 BackgroundSize = ImVec2(SquareSize.x, 4.f * SquareSize.y);

	auto [PromotionSquareMin, PromotionSquareMax] = GetSquare(Piece::RotateCCW(PawnPromotionSquare));

	if (PromotingPawn->Color == Black)
	{
		PromotionSquareMin.y -= 3.f * SquareSize.y;
	}

	ImGui::SetNextWindowPos(PromotionSquareMin);
	ImGui::SetNextWindowSize(BackgroundSize);
	ImGui::SetNextWindowFocus();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_PAWNPROMOTION_BG);

	ImGuiWindowFlags WindowFlags =
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoDecoration;

	if (ImGui::Begin("Pawn Promotion", nullptr, WindowFlags))
	{
		DrawList->ChannelsSetCurrent(1);

#define DRAW_TYPE(PieceType, Distance) \
const ImVec2 PieceType##Pos = ImVec2(PromotionSquareMin.x, PromotionSquareMin.y + (float)Distance * SquareSize.y); \
DrawPiece(PieceType##Pos, SquareSize, PieceType, Color);

		DRAW_TYPE(Queen,	0);
		DRAW_TYPE(Knight,	1);
		DRAW_TYPE(Rook,		2);
		DRAW_TYPE(Bishop,	3);

		DrawList->ChannelsSetCurrent(0);
	}
	ImGui::End();

	ImGui::PopStyleColor();
}

void ChessEngine::DrawEndScreen() const
{
	PieceColor WinningColor = (PieceColor)(1 - CurrentMove);
	const char* WinningText = WinningColor == White ? "White has won!\nClick to restart" : "Black has won!\nClick to restart";

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->ChannelsSetCurrent(1);

	DrawList->AddRectFilled(ImVec2(0.f, 0.f), ImGui::GetWindowSize(), COL_ENDSCREEN_OVERLAY);

	const ImGuiStyle& Style = ImGui::GetStyle();

	float OldFontScale = Style.FontSizeBase;

	const ImVec2 WindowSize = ImGui::GetWindowSize() - ImVec2(10.f, 10.f);
	const ImVec2 TextSize = ImGui::CalcTextSize(WinningText);
	const ImVec2 ScaleVec = WindowSize / TextSize;

	float Scale = min(ScaleVec.x, ScaleVec.y);

	const ImVec2 Cursor = (WindowSize - TextSize * Scale) / 2.f;

	ImGui::SetCursorScreenPos(Cursor);

	ImGui::PushFont(nullptr, OldFontScale * Scale);
	ImGui::Text("%s", WinningText);
	ImGui::PopFont();

	DrawList->ChannelsSetCurrent(0);
}

void ChessEngine::Update()
{
	HandleInput();

	if (NumPossibleMoves == -1)
	{
		CalculatePossibleMoves();
	}
}

void ChessEngine::Draw() const
{
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	DrawList->ChannelsSplit(2);
	DrawList->ChannelsSetCurrent(0);

#ifdef _DEBUG
	MarkedSquares.clear();
#endif

	DrawGrid();
	DrawMoveInfo();
	DrawSelectedPiece();
	DrawAvailableMoves();
	DrawPieces();
	DrawPromotionPopup();

	if (NumPossibleMoves == 0 && FullMoveCounter > 1)
	{
		DrawEndScreen();
	}

#ifdef _DEBUG
	DrawMarkedSquares();
#endif

	DrawList->ChannelsMerge();
}
