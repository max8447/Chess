#include "MoveGen.h"

static constexpr void InsertMoves(int StartSquare, uint64_t MovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	while (MovesBitboard)
	{
		int TargetSquare = pop_lsb(MovesBitboard);
		OutMoves->emplace_back(SimpleMove{ StartSquare, TargetSquare });
	}
}

template<GenType Type, PieceColor Color>
void GeneratePromotions(const Position& GamePosition, int StartSquare, int TargetSquare, std::vector<SpecialMove>* OutMoves)
{
	SpecialMoveType MoveType = PawnPromotion;

	if (IsBitSet(GamePosition.Bitboards.Occupied[~Color], TargetSquare))
	{
		MoveType |= Capture;
	}

	SimpleMove PawnMove{ StartSquare, TargetSquare };
	SimpleMove OtherPieceMove{ TargetSquare, -1 };

	OutMoves->emplace_back(PawnMove, MoveType, OtherPieceMove, nullptr /* for now */, Queen);
	OutMoves->emplace_back(PawnMove, MoveType, OtherPieceMove, nullptr /* for now */, Rook);
	OutMoves->emplace_back(PawnMove, MoveType, OtherPieceMove, nullptr /* for now */, Bishop);
	OutMoves->emplace_back(PawnMove, MoveType, OtherPieceMove, nullptr /* for now */, Knight);
}

template<PieceColor Color, GenType Type>
void GeneratePawnMoves(const Position& GamePosition, uint64_t LegalMovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	constexpr Direction MoveDir = Color == White ? North : South;
	constexpr int DoublePushRank = Color == White ? 1 : 6;
	constexpr int PromotionRank = Color == White ? 7 : 0;

	uint64_t PawnsBitboard = GamePosition.Bitboards.Pieces[Color][Pawn];

	while (PawnsBitboard)
	{
		const int PawnSquare = pop_lsb(PawnsBitboard);
		const auto [PawnRank, PawnFile] = Piece::SquareToRankFile(PawnSquare);

		const int ForwardSquare = PawnSquare + MoveDir;
		const int EnpassantPawnSquareLeft = ForwardSquare + West;
		const int EnpassantPawnSquareRight = ForwardSquare + East;

		uint64_t ForwardMovesBitboard = 0;
		uint64_t EnpassantMoveBitboard = 0;

		if (PawnRank != PromotionRank) // if we are on the promotion rank we can't go forward at all
		{
			if (EnpassantPawnSquareLeft == GamePosition.GameState.EnpassantSquare)
			{
				EnpassantMoveBitboard = ToBitboard(ForwardSquare + West);
			}
			else if (EnpassantPawnSquareRight == GamePosition.GameState.EnpassantSquare)
			{
				EnpassantMoveBitboard = ToBitboard(ForwardSquare + East);
			}

			ForwardMovesBitboard |= ToBitboard(ForwardSquare);
		}

		if (PawnRank == DoublePushRank && !IsBitSet(GamePosition.Bitboards.AllOccupied, ForwardSquare))
		{
			ForwardMovesBitboard |= ToBitboard(ForwardSquare + MoveDir);
		}
		else if (PawnRank == PromotionRank)
		{
			// TODO
		}

		uint64_t AttacksBitboard =
			GamePosition.Attacks.GetPseudoAttacks(PawnSquare, Pawn, Color, GamePosition.Bitboards.AllOccupied);

		uint64_t MovesBitboard =
			(	(AttacksBitboard & GamePosition.Bitboards.Occupied[~Color]) |
				(ForwardMovesBitboard & ~GamePosition.Bitboards.AllOccupied) |
				EnpassantMoveBitboard)
			& LegalMovesBitboard;

		InsertMoves(PawnSquare, MovesBitboard, OutMoves);
	}
}

template<PieceColor Color, PieceType Type>
void GenerateMoves(const Position& GamePosition, uint64_t LegalMovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	uint64_t PiecesBitboard = GamePosition.Bitboards.Pieces[Color][Type];

	while (PiecesBitboard)
	{
		int OldSquare = pop_lsb(PiecesBitboard);

		uint64_t AttacksBitboard =
			GamePosition.Attacks.GetPseudoAttacks(OldSquare, Type, Color, GamePosition.Bitboards.AllOccupied) & LegalMovesBitboard;

		InsertMoves(OldSquare, AttacksBitboard, OutMoves);
	}
}

template<GenType Type, PieceColor Color>
void GenerateAll(const Position& GamePosition, std::vector<SpecialMove>* OutMoves)
{
	int KingSquare = GamePosition.GetKingSquare(Color);
	uint64_t Checkers = GamePosition.CheckingPieces;
	uint64_t LegalMovesBitboard = 0;

	if (Type != Evasions || !AreMultipleBitsSet(Checkers))
	{
		LegalMovesBitboard =
				Type == Evasions ?		GamePosition.Attacks.SquaresBetween[KingSquare][lsb(Checkers)]
			:	Type == NonEvasions ?	~GamePosition.Bitboards.Occupied[Color]
			:	Type == CapturesOnly ?	GamePosition.Bitboards.Occupied[~Color]
			:/* Type == Quiet */		~GamePosition.Bitboards.AllOccupied;

		GeneratePawnMoves<Color, Type>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Knight>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Bishop>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Rook>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Queen>(GamePosition, LegalMovesBitboard, OutMoves);
	}

	uint64_t KingLegalMoves = Type == Evasions ? ~GamePosition.Bitboards.Occupied[Color] : LegalMovesBitboard;

	InsertMoves(KingSquare, GamePosition.Attacks.KingAttacks[KingSquare] & KingLegalMoves, OutMoves);

	if constexpr (Type == Quiet || Type == NonEvasions)
	{
		if (GamePosition.CanCastle(Color & AnyCastling))
		{
			for (CastlingRights CastlingRight : { Color & KingSide, Color & QueenSide })
			{
				if (GamePosition.CanCastle(CastlingRight) && !GamePosition.IsCastlingBlocked(CastlingRight))
				{
					OutMoves->emplace_back(CastlingRight & KingSide ?
						StaticCastlingRights::KingSide[Color] : StaticCastlingRights::QueenSide[Color]);
				}
			}
		}
	}
}

template<GenType Type>
void GenerateMoves(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves)
{
	STATIC_ASSERT(Type != Legal);
	ASSERT((GamePosition.CheckingPieces != 0) == (Type == Evasions));
	ASSERT(GamePosition.IsInCheck(Color) == (Type == Evasions));

	return Color == White ?
		GenerateAll<Type, White>(GamePosition, OutMoves) :
		GenerateAll<Type, Black>(GamePosition, OutMoves);
}

template void GenerateMoves<Quiet>(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves);
template void GenerateMoves<CapturesOnly>(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves);
template void GenerateMoves<Evasions>(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves);
template void GenerateMoves<NonEvasions>(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves);

template<>
void GenerateMoves<Legal>(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves)
{
	int KingSquare = GamePosition.GetKingSquare(Color);

	uint64_t Checkers = GamePosition.CheckingPieces;
	uint64_t Pinned = GamePosition.PinnedPieces[Color] & GamePosition.Bitboards.Occupied[Color];

	if (Checkers != 0)
	{
		printf("is in check!\n");
		GenerateMoves<Evasions>(GamePosition, Color, OutMoves);
	}
	else
	{
		GenerateMoves<NonEvasions>(GamePosition, Color, OutMoves);
	}

	// todo: check legality of move (using pinned pieces)
}