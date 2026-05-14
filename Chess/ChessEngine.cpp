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
	: SelectedPiece(nullptr), FENString(FENString), TranspositionTable(1)
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
	bGameEnded = false;

	CurrentMove = White;
	AvailableCastlingRights = {};
	EnpassantSquare = -1;
	PawnPromotionSquare = -1;
	HalfMoveClock = 0;
	FullMoveCounter = 0; // starts at 1, however the FEN string will contain the correct count
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
				AvailableCastlingRights.KingSide[0].Invalidate();
				AvailableCastlingRights.KingSide[1].Invalidate();

				AvailableCastlingRights.QueenSide[0].Invalidate();
				AvailableCastlingRights.QueenSide[1].Invalidate();

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
				FENString--;

				while (isdigit(c = *FENString++))
				{
					HalfMoveClock *= 10;
					HalfMoveClock += c - '0';
				}

				bSetupHalfMoveClock = true;
			}
			else if (!bSetupFullMoveCounter)
			{
				FENString--;

				while (isdigit(c = *FENString++))
				{
					FullMoveCounter *= 10;
					FullMoveCounter += c - '0';
				}

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
						AvailableCastlingRights.KingSide[i].Invalidate();
					}
					if (!QueenSideEnabled[i])
					{
						AvailableCastlingRights.QueenSide[i].Invalidate();
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

	printf("fen pos: %s\n", GenerateFENPosition());
}

char* ChessEngine::GenerateFENPosition()
{
	constexpr size_t MaxFENSize = 3490;

	char Buf[MaxFENSize] = { 0 };
	int Idx = 0;

	for (int Rank = 7; Rank >= 0; Rank--)
	{
		for (int File = 0; File < 8; File++)
		{
			Piece* Piece = GetPiece(Rank, File);

			if (Piece)
			{
				char PieceTypeChar;

				switch (Piece->Type)
				{
				case King:
					PieceTypeChar = 'k';
					break;
				case Queen:
					PieceTypeChar = 'q';
					break;
				case Bishop:
					PieceTypeChar = 'b';
					break;
				case Knight:
					PieceTypeChar = 'n';
					break;
				case Rook:
					PieceTypeChar = 'r';
					break;
				case Pawn:
					PieceTypeChar = 'p';
					break;
				default:
					ASSERT(false, nullptr);
					break;
				}

				Buf[Idx++] = Piece->Color == White ? toupper(PieceTypeChar) : PieceTypeChar;
			}
			else if (Idx > 0 && isdigit(Buf[Idx - 1]))
			{
				Buf[Idx - 1]++;
			}
			else
			{
				Buf[Idx++] = '1';
			}
		}

		Buf[Idx++] = '/';
	}

	Idx--; // last rank doesn't need a slash

	Buf[Idx++] = ' ';
	Buf[Idx++] = CurrentMove == White ? 'w' : 'b';
	Buf[Idx++] = ' ';

	bool bHasCastlingRights = false;

	if (AvailableCastlingRights.KingSide[White].Move.IsAllowed())
	{
		bHasCastlingRights = true;
		Buf[Idx++] = 'K';
	}
	if (AvailableCastlingRights.QueenSide[White].Move.IsAllowed())
	{
		bHasCastlingRights = true;
		Buf[Idx++] = 'Q';
	}
	if (AvailableCastlingRights.KingSide[Black].Move.IsAllowed())
	{
		bHasCastlingRights = true;
		Buf[Idx++] = 'k';
	}
	if (AvailableCastlingRights.QueenSide[Black].Move.IsAllowed())
	{
		bHasCastlingRights = true;
		Buf[Idx++] = 'q';
	}

	if (!bHasCastlingRights)
	{
		Buf[Idx++] = '-';
	}

	Buf[Idx++] = ' ';

	if (EnpassantSquare == -1)
	{
		Buf[Idx++] = '-';
	}
	else
	{
		std::array<char, 3> EnpassantSquareAlgebraic = Piece::SquareToAlgebraic(EnpassantSquare);

		Buf[Idx++] = EnpassantSquareAlgebraic[0];
		Buf[Idx++] = EnpassantSquareAlgebraic[1];
	}

	Buf[Idx++] = ' ';

	{
		char NumBuf[3] = { 0 };

		_itoa_s(HalfMoveClock, NumBuf, 10);

		size_t NumBufLen = strlen(NumBuf);
		memcpy(Buf + Idx, NumBuf, NumBufLen);
		Idx += NumBufLen;
	}

	Buf[Idx++] = ' ';

	// TODO
	//{
	//	char NumBuf[3] = { 0 };

	//	_itoa_s(FullMoveCounter, NumBuf, 10);

	//	size_t NumBufLen = strlen(NumBuf);
	//	memcpy(Buf + Idx, NumBuf, NumBufLen);
	//	Idx += NumBufLen;
	//}

	//Buf[Idx++] = '\0';

	return Buf;
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

void ChessEngine::GetAllScoredMoves(PieceColor Color, std::vector<ScoredMove>* OutScoredMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(OutScoredMoves);

	for (const auto& Piece : Pieces)
	{
		if (Piece->Color == Color && Piece->Square != -1)
		{
			GetScoredMoves(Piece.get(), OutScoredMoves, bAllowPseudolegalMoves);
		}
	}
}

void ChessEngine::GetAllAvailableMoves(PieceColor Color, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(OutAvailableMoves);

	for (const auto& Piece : Pieces)
	{
		if (Piece->Color == Color && Piece->Square != -1)
		{
			GetAvailableMoves(Piece.get(), OutAvailableMoves, bAllowPseudolegalMoves);
		}
	}
}

void ChessEngine::GetAllAvailableMoves(PieceColor Color, SpecialMove* OutAvailableMoves, int& OutAvailableMovesCount, bool bAllowPseudolegalMoves) const
{
	ASSERT(OutAvailableMoves);

	for (const auto& Piece : Pieces)
	{
		if (Piece->Color == Color && Piece->Square != -1)
		{
			GetAvailableMoves(Piece.get(), OutAvailableMoves, OutAvailableMovesCount, bAllowPseudolegalMoves);
		}
	}
}

void ChessEngine::GetScoredMoves(Piece* TargetPiece, std::vector<ScoredMove>* OutScoredMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(TargetPiece && OutScoredMoves);

	IterateSquares([&OutScoredMoves, TargetPiece, bAllowPseudolegalMoves, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int Square = Piece::RankFileToSquare(Rank, File);

			SpecialMove OutSpecialMove;
			if (IsAllowedMove(TargetPiece, Square, bAllowPseudolegalMoves, &OutSpecialMove) && OutSpecialMove.IsAllowed())
			{
				if (OutSpecialMove.Type == PawnPromotion || OutSpecialMove.Type == PawnPromotionCapture)
				{
					for (PieceType Type = Queen; Type < Pawn; Type = (PieceType)(Type + 1))
					{
						OutSpecialMove.PromotedPieceType = Type;
						OutScoredMoves->push_back({ .Move = OutSpecialMove, .Score = GuessMoveScore(OutSpecialMove) });
					}
				}
				else
				{
					OutScoredMoves->push_back({ .Move = OutSpecialMove, .Score = GuessMoveScore(OutSpecialMove) });
				}
			}

			return true;
		});
}

void ChessEngine::GetAvailableMoves(Piece* TargetPiece, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves) const
{
	ASSERT(TargetPiece && OutAvailableMoves);

	IterateSquares([&OutAvailableMoves, TargetPiece, bAllowPseudolegalMoves, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int Square = Piece::RankFileToSquare(Rank, File);

			SpecialMove OutSpecialMove;
			if (IsAllowedMove(TargetPiece, Square, bAllowPseudolegalMoves, &OutSpecialMove) && OutSpecialMove.IsAllowed())
			{
				if (OutSpecialMove.Type == PawnPromotion || OutSpecialMove.Type == PawnPromotionCapture)
				{
					for (PieceType Type = Queen; Type < Pawn; Type = (PieceType)(Type + 1))
					{
						OutSpecialMove.PromotedPieceType = Type;
						OutAvailableMoves->push_back(OutSpecialMove);
					}
				}
				else
				{
					OutAvailableMoves->push_back(OutSpecialMove);
				}
			}

			return true;
		});
}

void ChessEngine::GetAvailableMoves(Piece* TargetPiece, SpecialMove* OutAvailableMoves, int& OutAvailableMovesCount, bool bAllowPseudolegalMoves) const
{
	ASSERT(TargetPiece && OutAvailableMoves);

	IterateSquares([&OutAvailableMoves, &OutAvailableMovesCount, TargetPiece, bAllowPseudolegalMoves, this](const ImVec2& Min, const ImVec2& Max, int Rank, int File) -> bool
		{
			int Square = Piece::RankFileToSquare(Rank, File);

			SpecialMove OutSpecialMove;
			if (IsAllowedMove(TargetPiece, Square, bAllowPseudolegalMoves, &OutSpecialMove) && OutSpecialMove.IsAllowed())
			{
				if (OutSpecialMove.Type == PawnPromotion || OutSpecialMove.Type == PawnPromotionCapture)
				{
					for (PieceType Type = Queen; Type < Pawn; Type = (PieceType)(Type + 1))
					{
						OutSpecialMove.PromotedPieceType = Type;
						OutAvailableMoves[OutAvailableMovesCount++] = OutSpecialMove;
					}
				}
				else
				{
					OutAvailableMoves[OutAvailableMovesCount++] = OutSpecialMove;
				}
			}

			return true;
		});
}

void ChessEngine::CalculatePossibleMoves()
{
	bool bAllowPseudolegal = false;

	constexpr size_t MaxAvailableMoves = 218; // https://www.chessprogramming.org/Chess_Position

	AllAvailableMoves.clear();
	AllAvailableMoves.reserve(MaxAvailableMoves);

	GetAllAvailableMoves(CurrentMove, &AllAvailableMoves, bAllowPseudolegal);

	NumPossibleMoves = AllAvailableMoves.size();

	if (NumPossibleMoves == 0)
	{
		bGameEnded = true;
	}

	if constexpr (false)
	{
		using namespace std::chrono;

		steady_clock::time_point IsAllowedMoveStart = high_resolution_clock::now();

		int Depth = 5;
		MoveGenerationTestPossibleMoves = MoveGenerationTest(CurrentMove, Depth, true);

		steady_clock::time_point IsAllowedMoveEnd = high_resolution_clock::now();
		milliseconds IsAllowedMoveDuration = duration_cast<milliseconds>(IsAllowedMoveEnd - IsAllowedMoveStart);

		printf("Nodes searched at depth %d: %d\n", Depth, MoveGenerationTestPossibleMoves);
		printf("MoveGenerationTest took %lld ms\n", IsAllowedMoveDuration.count());
	}
}

bool ChessEngine::IsAllowedMove(Piece* MovingPiece, int NewSquare, bool bAllowPseudolegal, SpecialMove* OutSpecialMove) const
{
	ASSERT(MovingPiece && OutSpecialMove, false);
	ASSERT(NewSquare >= 0 && NewSquare <= 63, false);

	if (MovingPiece->Square == NewSquare)
	{
		return false; // no move happened
	}

	PieceType OldNewPieceType = OutSpecialMove->PromotedPieceType;

	OutSpecialMove->Invalidate();
	OutSpecialMove->Move = { MovingPiece->Square, NewSquare};
	OutSpecialMove->MovedPiece = MovingPiece;
	OutSpecialMove->PromotedPieceType = OldNewPieceType;

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
		else if (CapturedPiece->Type == King)
		{
			CapturedPiece = nullptr;
			bIsMoveAllowed = false;
		}
		else
		{
			OutSpecialMove->Type = Capture;
			OutSpecialMove->OtherPieceMove = { NewSquare, -1 };
			OutSpecialMove->CapturedPiece = CapturedPiece;
		}
	}

	if (!bHasPieceOfSameColor)
	{
		if (MovingPiece->Type == Pawn)
		{
			if (DeltaRank == 1 && AbsDeltaFile == 1)
			{
				if (CapturedPiece) // normal pawn capture
				{
					bIsMoveAllowed = true;
				}
				else if (EnpassantSquare != -1) // en passant capture
				{
					int EnpassantSquareToTest = MovingPiece->GetSquare(OldRank, NewFile);

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
								OutSpecialMove->bIsEnpassant = true;
							}
							else
							{
								CapturedPiece = nullptr;
								bIsMoveAllowed = false;
							}
						}
					}
				}
			}
			else if (AbsDeltaRank == 2 && AbsDeltaFile == 0 && OldRank == 1 && !CapturedPiece) // pawn's first move
			{
				int SquareInFront = MovingPiece->GetSquare(OldRank + 1, OldFile);

				Piece* PieceInFront = GetPiece(SquareInFront);

				bIsMoveAllowed = PieceInFront == nullptr;

				if (bIsMoveAllowed)
				{
					OutSpecialMove->Type = PawnDoublePush;
					OutSpecialMove->OtherPieceMove = OutSpecialMove->Move;
					OutSpecialMove->EnpassantSquare = NewSquare;
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

			int LineToCheck = AbsDeltaRank == 0 ? AbsDeltaFile : AbsDeltaRank;

			for (int i = 1; i < LineToCheck + 1; i++)
			{
				int CurrentRank = OldRank + i * DeltaRankDir;
				int CurrentFile = OldFile + i * DeltaFileDir;

				if (CurrentRank < 0 || CurrentRank > 7 ||
					CurrentFile < 0 || CurrentFile > 7)
				{
					break;
				}

				int CurrentSquare = MovingPiece->GetSquare(CurrentRank, CurrentFile);

				if (bRayHitPiece)
				{
					bIsMoveAllowed = false;
					break;
				}

				if (Piece* SkippedPiece = GetPiece(CurrentSquare))
				{
					if (SkippedPiece->Color == MovingPiece->Color)
					{
						bIsMoveAllowed = false;
						break;
					}

					bRayHitPiece = true;
				}

				CurrentRank += DeltaRankDir;
				CurrentFile += DeltaFileDir;
			}
		}
		else if (MovingPiece->Type == King)
		{
			if (!bIsMoveAllowed && !CapturedPiece) // might be trying to castle?
			{
				if (NewFile + DeltaFileDir >= 0 && NewFile + DeltaFileDir <= 7)
				{
					int CastlingRookSquare = MovingPiece->GetSquare(OldRank, NewFile + DeltaFileDir);
					Piece* CastlingRook = GetPiece(CastlingRookSquare);

					if (!CastlingRook)
					{
						int CastlingRookFile = NewFile + 2 * DeltaFileDir;

						if (CastlingRookFile >= 0 && CastlingRookFile <= 7)
						{
							CastlingRookSquare = MovingPiece->GetSquare(OldRank, CastlingRookFile);
							CastlingRook = GetPiece(CastlingRookSquare);
						}
					}

					if (CastlingRook && CastlingRook->Type == Rook && CastlingRook->Color == MovingPiece->Color)
					{
						int SquareBetween = MovingPiece->GetSquare(OldRank, OldFile + DeltaFileDir);
						Piece* PieceBetween = GetPiece(SquareBetween);

						if (!PieceBetween)
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

							const SpecialMove& KingSideCastle = AvailableCastlingRights.KingSide[MovingPiece->Color];
							const SpecialMove& QueenSideCastle = AvailableCastlingRights.QueenSide[MovingPiece->Color];

							if (KingSideCastle.Type == Castle && KingSideCastle.Move.IsAllowed() && NewSquare == KingSideCastle.Move.NewSquare)
							{
								AllowCastle();
							}
							else if (QueenSideCastle.Type == Castle && QueenSideCastle.Move.IsAllowed() && NewSquare == QueenSideCastle.Move.NewSquare)
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
	}

	if (bIsMoveAllowed)
	{
		if (MovingPiece->Type == Pawn && NewRank == 7) // Pawn promotion
		{
			if (OutSpecialMove->Type == Capture)
			{
				OutSpecialMove->Type = PawnPromotionCapture;
			}
			else
			{
				OutSpecialMove->Type = PawnPromotion;
			}
		}

		if (!bAllowPseudolegal)
		{
			// don't allow the move if it leaves us in check after

			CastlingRights CastlingRights = AvailableCastlingRights;
			int CurrentEnpassantSquare = EnpassantSquare;

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

	return bIsMoveAllowed;
}

void ChessEngine::MakeMove(const SpecialMove& Move, CastlingRights* OutCastlingRights, int* OutEnpassantSquare, uint64_t* OutHash) const
{
	ASSERT(Move.MovedPiece && Move.Move.IsAllowed());

	Move.MovedPiece->Square = Move.Move.NewSquare;

	Piece* OtherPiece = nullptr;

	if (Move.IsAllowed())
	{
		if (Move.Type == Castle)
		{
			OtherPiece = Move.CastledRook;
		}
		else if (Move.Type == Capture)
		{
			OtherPiece = Move.CapturedPiece;
		}
		else if (Move.Type == PawnPromotion)
		{
			Move.MovedPiece->Type = Move.PromotedPieceType;
		}
		else if (Move.Type == PawnPromotionCapture)
		{
			OtherPiece = Move.CapturedPiece;
			Move.MovedPiece->Type = Move.PromotedPieceType;
		}
	}

	if (OtherPiece)
	{
		OtherPiece->Square = Move.OtherPieceMove.NewSquare;

		if ((OtherPiece->Type == King || OtherPiece->Type == Rook) && OutCastlingRights)
		{
			RemoveCastlingRights(OtherPiece, OtherPiece->Square, *OutCastlingRights);
		}
	}

	if ((Move.MovedPiece->Type == King || Move.MovedPiece->Type == Rook) && OutCastlingRights)
	{
		RemoveCastlingRights(Move.MovedPiece, Move.MovedPiece->Square, *OutCastlingRights);
	}

	if (OutEnpassantSquare)
	{
		*OutEnpassantSquare = Move.EnpassantSquare;
	}

	if (OutHash && OutCastlingRights && OutEnpassantSquare)
	{
		*OutHash = GenerateHash((PieceColor)(1 - CurrentMove), *OutCastlingRights, *OutEnpassantSquare);
	}
}

void ChessEngine::UnMakeMove(const SpecialMove& Move) const
{
	ASSERT(Move.MovedPiece && Move.Move.IsAllowed());

	Move.MovedPiece->Square = Move.Move.OldSquare;

	Piece* OtherPiece = nullptr;

	// don't check move validity since it might have changed in MakeMove

	if (Move.Type == Castle)
	{
		OtherPiece = Move.CastledRook;
	}
	else if (Move.Type == Capture)
	{
		OtherPiece = Move.CapturedPiece;
	}
	else if (Move.Type == PawnPromotion)
	{
		Move.MovedPiece->Type = Pawn;
	}
	else if (Move.Type == PawnPromotionCapture)
	{
		OtherPiece = Move.CapturedPiece;
		Move.MovedPiece->Type = Pawn;
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

			if (CurrentRank < 0 || CurrentRank > 7 ||
				CurrentFile < 0 || CurrentFile > 7)
			{
				continue;
			}

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

void ChessEngine::RemoveCastlingRights(Piece* MovedPiece, int NewSquare, CastlingRights& OutCastlingRights) const
{
	ASSERT(MovedPiece);

	PieceColor Color = MovedPiece->Color;

	if (MovedPiece->Type == King)
	{
		OutCastlingRights.KingSide[Color].Invalidate();
		OutCastlingRights.QueenSide[Color].Invalidate();
	}
	else if (MovedPiece->Type == Rook)
	{
		if (MovedPiece->Square == OutCastlingRights.KingSide[Color].OtherPieceMove.NewSquare)
		{
			OutCastlingRights.KingSide[Color].Invalidate();
		}
		else if (MovedPiece->Square == OutCastlingRights.QueenSide[Color].OtherPieceMove.NewSquare)
		{
			OutCastlingRights.QueenSide[Color].Invalidate();
		}
	}
}

void ChessEngine::FinishMove(Piece* MovingPiece, const SpecialMove& Move)
{
	ASSERT(MovingPiece && Move.IsAllowed());

	if (CurrentMove == Black)
	{
		FullMoveCounter++;
	}

	if (MovingPiece->Type == Pawn)
	{
		HalfMoveClock = 0;
	}

	if (Move.Type == PawnDoublePush)
	{
		EnpassantSquare = Move.EnpassantSquare;
	}
	else if (Move.Type == Castle && Move.OtherPieceMove.IsAllowed())
	{
		ASSERT(Move.CastledRook);

		Move.CastledRook->Square = Move.OtherPieceMove.NewSquare;
	}

	if (PawnPromotionSquare == -1 && (Move.Type == PawnPromotion || Move.Type == PawnPromotionCapture))
	{
		PawnPromotionSquare = Move.Move.NewSquare;
	}
	else
	{
		PawnPromotionSquare = -1;
	}

	LastMove = Move;
	MovingPiece->Square = Move.Move.NewSquare;

	if (PawnPromotionSquare == -1)
	{
		CurrentMove = (PieceColor)(1 - CurrentMove);
	}

	CalculatePossibleMoves();

	if (HalfMoveClock >= 100) // it should actually only enforce it at 150 and offer draw at 100 but it's fine for now (todo)
	{
		bGameEnded = true;
	}
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
			RemoveCastlingRights(MovingPiece, NewSquare, AvailableCastlingRights);
		}
		
		if (OutSpecialMove.Type == Capture || OutSpecialMove.Type == PawnPromotionCapture)
		{
			Piece* CapturedPiece = OutSpecialMove.CapturedPiece;

			if (CapturedPiece)
			{
				if (CapturedPiece->Type == King || CapturedPiece->Type == Rook) // can never be king but you never know
				{
					RemoveCastlingRights(CapturedPiece, CapturedPiece->Square, AvailableCastlingRights);
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

void ChessEngine::HandleBotPawnPromotion()
{
	if (PawnPromotionSquare == -1)
	{
		return;
	}

	Piece* PromotingPawn = GetPiece(PawnPromotionSquare);

	ASSERT(PromotingPawn);

	PieceType OldPieceType = PromotingPawn->Type;

	ASSERT(OldPieceType == Pawn);

	PieceType PromotedPieceType = Queen; // TODO: choose highest for now

	LastMove.PromotedPieceType = PromotedPieceType;

	SelectedPieceMouseOffset = { -FLT_MAX, FLT_MAX };
	SelectedPiece = nullptr;

	FinishMove(PromotingPawn, LastMove);

	PromotingPawn->Type = PromotedPieceType;
}

void ChessEngine::GenerateMove()
{
	if (CurrentMove != BotColor)
	{
		return;
	}

	if (PawnPromotionSquare != -1)
	{
		HandleBotPawnPromotion();
		return;
	}

	if (AllAvailableMoves.empty())
	{
		CalculatePossibleMoves();
	}

	if (AllAvailableMoves.empty())
	{
		return; // checkmate, bot lost
	}

	using namespace std::chrono;

	steady_clock::time_point SearchBestMoveStart = high_resolution_clock::now();
	
	CurrentHash = GenerateHash(CurrentMove, AvailableCastlingRights, EnpassantSquare);

	printf("currenthash: 0x%llX\n", CurrentHash);

	int Depth = 4;

	SpecialMove ChosenMove;
	int BestScore = SearchBestMove(ChosenMove, BotColor, Depth);

	steady_clock::time_point SearchBestMoveEnd = high_resolution_clock::now();
	milliseconds SearchBestMoveDuration = duration_cast<milliseconds>(SearchBestMoveEnd - SearchBestMoveStart);

	printf("SearchBestMove took %lld ms\n", SearchBestMoveDuration.count());

	if (ChosenMove.IsAllowed())
	{
		printf("playing move with score %d\n", BestScore);
		TryMoveTo(ChosenMove.MovedPiece, ChosenMove.Move.NewSquare);
	}
	//else if (BestScore > SEARCHBESTMOVE_MIN)
	//{
	//	INT3;
	//}
}

int ChessEngine::GetMaterialValue(PieceColor Color) const
{
	int Value = 0;

	for (const auto& Piece : Pieces)
	{
		if (Piece->Color != Color || Piece->Type == King) // don't count kings
		{
			continue;
		}

		Value += PieceValues[Piece->Type];
	}

	return Value;
}

int ChessEngine::GetMobility(PieceColor Color) const
{
	constexpr size_t MaxAvailableMoves = 218; // https://www.chessprogramming.org/Chess_Position

	std::vector<SpecialMove> PossibleMoves;
	PossibleMoves.reserve(MaxAvailableMoves);

	constexpr bool bAllowPseudolegalMoves = false;
	GetAllAvailableMoves(Color, &PossibleMoves, bAllowPseudolegalMoves);

	return PossibleMoves.size();
}

int ChessEngine::EvaluatePosition() const
{
	int WhiteMaterialValue = GetMaterialValue(White);
	int BlackMaterialValue = GetMaterialValue(Black);

	int MaterialScore = WhiteMaterialValue - BlackMaterialValue;

	int WhiteMobility = GetMobility(White);
	int BlackMobility = GetMobility(Black);

	int MobilityScore = WhiteMobility - BlackMobility;

	int Perspective = CurrentMove == White ? 1 : -1;

	return (MaterialScore + MobilityScore) * Perspective;
}

int ChessEngine::GuessMoveScore(const SpecialMove& Move) const
{
	int Score = 0;

	if (Move.CapturedPiece)
	{
		Score += 10 * PieceValues[Move.CapturedPiece->Type]
			- PieceValues[Move.MovedPiece->Type];
	}

	if (Move.Type == PawnPromotion ||
		Move.Type == PawnPromotionCapture)
	{
		Score += 1000 + PieceValues[Move.PromotedPieceType];
	}

	return Score;
}

int ChessEngine::SearchBestMove(SpecialMove& OutBestMove, PieceColor Color, int Depth, int Alpha, int Beta) const
{
	if (ZobrishEntry* Entry = TranspositionTable.GetEntry(CurrentHash))
	{
		if (Entry->Depth >= Depth)
		{
			switch (Entry->Flag)
			{
			case EXACT:
				return Entry->Score;
			case LOWERBOUND:
				Alpha = max(Alpha, Entry->Score);
				break;
			case UPPERBOUND:
				Beta = min(Beta, Entry->Score);
				break;
			default:
				__assume(false);
				break;
			}

			if (Alpha >= Beta)
			{
				return Entry->Score;
			}
		}
	}

	if (Depth == 0)
	{
		return EvaluatePosition();
	}

	constexpr size_t MaxAvailableMoves = 218; // https://www.chessprogramming.org/Chess_Position

	std::vector<ScoredMove> Moves;
	Moves.reserve(MaxAvailableMoves);

	constexpr bool bAllowPseudolegalMoves = true; // check legality later
	GetAllScoredMoves(Color, &Moves, bAllowPseudolegalMoves);

	if (Moves.size() == 0)
	{
		if (IsInCheck(Color))
		{
			return SEARCHBESTMOVE_MIN + Depth; // checkmate (lost)
		}

		return 0; // stalemate
	}

	// move ordering
	std::sort(std::execution::par_unseq, Moves.begin(), Moves.end(), [this](const ScoredMove& A, const ScoredMove& B) -> bool
		{
			return A.Score > B.Score;
		});

	int OriginalAlpha = Alpha;

	for (const ScoredMove& Move : Moves)
	{
		CastlingRights OldCastlingRights = AvailableCastlingRights;
		int OldEnpassantSquare = EnpassantSquare;
		uint64_t OldHash = CurrentHash;

		MakeMove(Move.Move, &AvailableCastlingRights, &EnpassantSquare, &CurrentHash);

		int MoveEval = SEARCHBESTMOVE_MIN;

		if (!IsInCheck(Color))
		{
			SpecialMove ChildBestMove;
			MoveEval = -SearchBestMove(ChildBestMove, (PieceColor)(1 - Color), Depth - 1, -Beta, -Alpha);
		}

		UnMakeMove(Move.Move);

		CurrentHash = OldHash;
		EnpassantSquare = OldEnpassantSquare;
		AvailableCastlingRights = OldCastlingRights;

		if (MoveEval >= Beta)
		{
			TranspositionTable.Store(CurrentHash, Depth, Alpha, LOWERBOUND);

			return Beta;
		}

		if (MoveEval > Alpha)
		{
			Alpha = MoveEval;
			OutBestMove = Move.Move;
		}
	}

	HashFlag Flag;

	if (Alpha <= OriginalAlpha)
	{
		Flag = UPPERBOUND;
	}
	else if (Alpha >= Beta)
	{
		Flag = LOWERBOUND;
	}
	else
	{
		Flag = EXACT;
	}

	TranspositionTable.Store(CurrentHash, Depth, Alpha, Flag);

	return Alpha;
}

static int Captures = 0, Ep = 0, Castles = 0, Promotions = 0;

int ChessEngine::MoveGenerationTest(PieceColor Color, int Depth, bool bIsRoot)
{
	if (Depth == 0)
	{
		return 1;
	}

	std::vector<SpecialMove> Moves;
	constexpr size_t MaxAvailableMoves = 218; // https://www.chessprogramming.org/Chess_Position

	Moves.reserve(MaxAvailableMoves);

	constexpr bool bAllowPseudolegalMoves = false;

	GetAllAvailableMoves(Color, &Moves, bAllowPseudolegalMoves);

	int NumPositons = 0;

	for (const SpecialMove& Move : Moves)
	{
		CastlingRights OldCastlingRights = AvailableCastlingRights;
		int OldEnpassantSquare = EnpassantSquare;

		MakeMove(Move, &AvailableCastlingRights, &EnpassantSquare);

		if (Depth == 1)
		{
			NumPositons++;

			if (Move.Type == Capture)
			{
				Captures++;

				if (Move.bIsEnpassant)
				{
					Ep++;
				}
			}
			else if (Move.Type == PawnPromotion)
			{
				Promotions++;
			}
			else if (Move.Type == PawnPromotionCapture)
			{
				Captures++;
				Promotions++;
			}
			else if (Move.Type == Castle)
			{
				Castles++;
			}
		}
		else
		{
			int PositionsFound = MoveGenerationTest((PieceColor)(1 - Color), Depth - 1, false);
			NumPositons += PositionsFound;

			if (bIsRoot)
			{
				printf("%02d -> %02d: %d\n", Move.Move.OldSquare, Move.Move.NewSquare, PositionsFound);
			}
		}

		UnMakeMove(Move);

		EnpassantSquare = OldEnpassantSquare;
		AvailableCastlingRights = OldCastlingRights;
	}

	if (bIsRoot)
	{
		printf("Captures = %d, Ep = %d, Castles = %d, Promotions = %d\n",
			Captures, Ep, Castles, Promotions);
	}

	return NumPositons;
}

uint64_t ChessEngine::GenerateHash(PieceColor Move, const CastlingRights& CastlingRights, int EnpassantSquare) const
{
	uint64_t Hash = 0;

	for (const auto& Piece : Pieces)
	{
		if (Piece->Type != Null)
		{
			Hash ^= TranspositionTable.PieceKeys[Piece->Type][Piece->Square];
		}
	}

	if (Move == Black)
	{
		Hash ^= TranspositionTable.SideToMoveKey;
	}

	int CastlingRightsKey = 0;

	if (CastlingRights.KingSide[White].Move.IsAllowed())
	{
		CastlingRightsKey |= 0b0001;
	}
	if (CastlingRights.QueenSide[White].Move.IsAllowed())
	{
		CastlingRightsKey |= 0b0010;
	}
	if (CastlingRights.KingSide[Black].Move.IsAllowed())
	{
		CastlingRightsKey |= 0b0100;
	}
	if (CastlingRights.QueenSide[Black].Move.IsAllowed())
	{
		CastlingRightsKey |= 0b1000;
	}

	Hash ^= TranspositionTable.CastlingKeys[CastlingRightsKey];

	if (EnpassantSquare != -1)
	{
		Hash ^= TranspositionTable.EnpassantKeys[EnpassantSquare];
	}

	return Hash;
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
			if (SelectedPiece->Color != CurrentMove /* || SelectedPiece->Color != PlayerColor */)
			{
				SelectedPiece = nullptr;
			}
			else // generate all legal moves once to be used later on
			{
				bool bAllowPseudolegalMoves = false;

				constexpr size_t MaxAvailableMoves = 27; // a queen in the middle of the board

				AvailableMoves.clear();
				AvailableMoves.reserve(MaxAvailableMoves);

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

			ASSERT(PromotingPawn && (LastMove.Type == PawnPromotion || LastMove.Type == PawnPromotionCapture));

			PieceType OldPieceType = PromotingPawn->Type;

			ASSERT(OldPieceType == Pawn);

			int OldSquare = LastMove.Move.OldSquare;

			const ImVec2 SquareSize = GetSquareSize();
			auto [PromotionSquareMin, PromotionSquareMax] = GetSquare(Piece::RotateCCW(PawnPromotionSquare));

			if (PromotingPawn->Color == Black)
			{
				PromotionSquareMin.y -= 3.f * SquareSize.y;
			}

			if (bLeftMouseClicked)
			{
				bool bPromoted = false;

				PieceType PromotedPieceType = Null;

#define CHECK_FOR(PieceType, Distance) \
const ImVec2 PieceType##Pos = ImVec2(PromotionSquareMin.x, PromotionSquareMin.y + (float)Distance * SquareSize.y); \
if (MousePos >= PieceType##Pos && MousePos <= PieceType##Pos + SquareSize) { PromotedPieceType = PieceType; bPromoted = true; }

				CHECK_FOR(Queen, 0);
				CHECK_FOR(Knight, 1);
				CHECK_FOR(Rook, 2);
				CHECK_FOR(Bishop, 3);

#undef CHECK_FOR

				if (bPromoted)
				{
					LastMove.PromotedPieceType = PromotedPieceType;
					
					SelectedPieceMouseOffset = { -FLT_MAX, FLT_MAX };
					SelectedPiece = nullptr;

					FinishMove(PromotingPawn, LastMove);

					PromotingPawn->Type = PromotedPieceType;
				}
			}
		}
	}
	else if (bGameEnded)
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

	// AvailableCastlingRights

	constexpr bool bDrawAvailableCastlingRights = false;

	if constexpr (bDrawAvailableCastlingRights)
	{
#ifdef _DEBUG
		if (AvailableCastlingRights.KingSide[0].Move.IsAllowed())
		{
			MarkedSquares.push_back(AvailableCastlingRights.KingSide[0].Move.NewSquare);
		}

		if (AvailableCastlingRights.KingSide[1].Move.IsAllowed())
		{
			MarkedSquares.push_back(AvailableCastlingRights.KingSide[1].Move.NewSquare);
		}

		if (AvailableCastlingRights.QueenSide[0].Move.IsAllowed())
		{
			MarkedSquares.push_back(AvailableCastlingRights.QueenSide[0].Move.NewSquare);
		}

		if (AvailableCastlingRights.QueenSide[1].Move.IsAllowed())
		{
			MarkedSquares.push_back(AvailableCastlingRights.QueenSide[1].Move.NewSquare);
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

	if (MoveGenerationTestPossibleMoves != -1)
	{
		ImGui::SetCursorScreenPos(ImVec2(CurrentMoveSquareMin.x - PADDING.x - 100.f, ImGui::GetCursorPosY()));
		ImGui::Text("MoveGenerationTestPossibleMoves: %d", MoveGenerationTestPossibleMoves);
	}

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

	std::unordered_set<int> DrawnSquares;

	for (const SpecialMove& Move : AvailableMoves)
	{
		int NewSquare = Move.Move.NewSquare;

		if (DrawnSquares.contains(NewSquare))
		{
			continue;
		}

		DrawnSquares.insert(NewSquare);

		const auto [MoveMin, MoveMax] = GetSquare(Piece::RotateCCW(NewSquare));

		float Diameter = (MoveMax - MoveMin).x;

		if (Move.Type == Capture || Move.Type == PawnPromotionCapture)
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
	ASSERT(PawnPromotionSquare != -1);

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

#undef DRAW_TYPE

		DrawList->ChannelsSetCurrent(0);
	}
	ImGui::End();

	ImGui::PopStyleColor();
}

void ChessEngine::DrawEndScreen() const
{
	PieceColor LosingColor = CurrentMove;
	PieceColor WinningColor = (PieceColor)(1 - LosingColor);
	
	enum EndingState
	{
		Checkmate,
		Stalemate,
		Draw
	};

	EndingState EndingState;

	if (IsInCheck(LosingColor))
	{
		EndingState = Checkmate;
	}
	else if (NumPossibleMoves == 0)
	{
		EndingState = Stalemate;
	}
	else
	{
		EndingState = Draw;
	}

	const char* WinnerType = nullptr;

	switch (EndingState)
	{
	case Checkmate:
		WinnerType = WinningColor == White ?
			"White has won by checkmate" :
			"Black has won by checkmate";
		break;
	case Stalemate:
		WinnerType = "Draw by stalemate";
		break;
	case Draw:
		WinnerType = "Draw";
		break;
	default:
		break;
	}

	char WinningText[64] = { 0 };
	sprintf_s(WinningText, "%s!\nClick to restart", WinnerType);

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
	if (CurrentMove == PlayerColor || bGameEnded)
	{
		HandleInput();
	}
	else if (CurrentMove == BotColor)
	{
		GenerateMove();
	}

	if (NumPossibleMoves == -1 || MoveGenerationTestPossibleMoves == -1)
	{
		CalculatePossibleMoves();
	}
	
	if (NumPossibleMoves == 0)
	{
		bGameEnded = true;
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

	if (!bGameEnded)
	{
		DrawAvailableMoves();
	}

	DrawPieces();

	if (PawnPromotionSquare != -1 && CurrentMove == PlayerColor)
	{
		DrawPromotionPopup();
	}

	if (bGameEnded && FullMoveCounter > 1)
	{
		DrawEndScreen();
	}

#ifdef _DEBUG
	DrawMarkedSquares();
#endif

	DrawList->ChannelsMerge();
}
