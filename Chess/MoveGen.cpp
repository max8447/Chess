#include "MoveGen.h"

static constexpr void InsertMoves(int StartSquare, uint64_t MovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	while (MovesBitboard)
	{
		int TargetSquare = pop_lsb(MovesBitboard);
		OutMoves->emplace_back(SimpleMove{ StartSquare, TargetSquare });
	}
}

static constexpr void InsertCaptures(int StartSquare, uint64_t MovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	constexpr SpecialMoveType MoveType = Capture;

	while (MovesBitboard)
	{
		int TargetSquare = pop_lsb(MovesBitboard);

		SimpleMove Move{ StartSquare, TargetSquare };
		SimpleMove OtherPieceMove{ TargetSquare, -1 };

		OutMoves->emplace_back(Move, MoveType, OtherPieceMove, nullptr /* for now */);
	}
}

template<PieceColor Color, GenType Type>
void GeneratePromotions(const Position& GamePosition, int StartSquare, int TargetSquare, std::vector<SpecialMove>* OutMoves)
{
	SpecialMoveType MoveType = PawnPromotion;
	SimpleMove PawnMove{ StartSquare, TargetSquare };
	SimpleMove OtherPieceMove{};

	if (IsBitSet(GamePosition.Bitboards.Occupied[~Color], TargetSquare))
	{
		MoveType |= Capture;
		OtherPieceMove = { TargetSquare, -1 };
	}

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
	constexpr int PromotionMoveRank = Color == White ? 6 : 1;
	constexpr int PromotionRank = Color == White ? 7 : 0;
	constexpr uint64_t FileA = 0x0101010101010101;
	constexpr uint64_t FileH = 0x8080808080808080;
	constexpr SpecialMoveType EnpassantMoveType = Capture;

	const uint64_t AllOccupiedBitboard = GamePosition.Bitboards.AllOccupied;
	const uint64_t OpponentOccupiedBitboard = GamePosition.Bitboards.Occupied[~Color];

	const int8_t EnpassantSquare = GamePosition.GameState.EnpassantSquare;

	uint64_t PawnsBitboard = GamePosition.Bitboards.Pieces[Color][Pawn];

	while (PawnsBitboard)
	{
		const int PawnSquare = pop_lsb(PawnsBitboard);
		const auto [PawnRank, PawnFile] = Piece::SquareToRankFile(PawnSquare);

		const int ForwardSquare = PawnSquare + MoveDir;
		const int PromotionCaptureSquareLeft = ForwardSquare + West;
		const int PromotionCaptureSquareRight = ForwardSquare + East;
		const int EnpassantPawnSquareLeft = ForwardSquare + West;
		const int EnpassantPawnSquareRight = ForwardSquare + East;

		uint64_t ForwardMovesBitboard = 0;
		uint64_t PromotionMovesBitboard = 0;
		uint64_t EnpassantMoveBitboard = 0;

		if (PawnRank == PromotionMoveRank)
		{
			if (!IsBitSet(GamePosition.Bitboards.AllOccupied, ForwardSquare))
			{
				PromotionMovesBitboard |= ToBitboard(ForwardSquare);
			}

			if (IsBitSet(GamePosition.Bitboards.Occupied[~Color], PromotionCaptureSquareLeft))
			{
				PromotionMovesBitboard |= ToBitboard(PromotionCaptureSquareLeft);
			}

			if (IsBitSet(GamePosition.Bitboards.Occupied[~Color], PromotionCaptureSquareRight))
			{
				PromotionMovesBitboard |= ToBitboard(PromotionCaptureSquareRight);
			}
		}
		else
		{
			if ((EnpassantPawnSquareLeft == EnpassantSquare && !(ToBitboard(PawnSquare) & FileA)) ||
				(EnpassantPawnSquareRight == EnpassantSquare && !(ToBitboard(PawnSquare) & FileH)))
			{
				EnpassantMoveBitboard = ToBitboard(EnpassantSquare);

				SimpleMove EnpassantMove{ PawnSquare, EnpassantSquare };
				SimpleMove EnpassantOtherPieceMove{ EnpassantSquare, -1 };

				OutMoves->emplace_back(EnpassantMove, EnpassantMoveType, EnpassantOtherPieceMove, nullptr /* for now */);
			}

			// if we are on the promotion rank we can't go forward at all (we should never get to this point anyway)
			if (PawnRank != PromotionRank)
			{
				ForwardMovesBitboard = ToBitboard(ForwardSquare);

				if (PawnRank == DoublePushRank && !IsBitSet(AllOccupiedBitboard, ForwardSquare))
				{
					ForwardMovesBitboard |= ToBitboard(ForwardSquare + MoveDir);
				}
			}
		}

		uint64_t AttacksBitboard =
			GamePosition.Attacks.GetPseudoAttacks(PawnSquare, Pawn, Color) & LegalMovesBitboard;

		uint64_t CapturesBitboard = AttacksBitboard & OpponentOccupiedBitboard & ~PromotionMovesBitboard & ~EnpassantMoveBitboard;

		InsertCaptures(PawnSquare, CapturesBitboard, OutMoves);

		uint64_t MovesBitboard =
			ForwardMovesBitboard & ~AllOccupiedBitboard & LegalMovesBitboard;

		InsertMoves(PawnSquare, MovesBitboard, OutMoves);

		uint64_t PromotionsBitboard =
			PromotionMovesBitboard & LegalMovesBitboard;

		while (PromotionsBitboard)
		{
			const int TargetSquare = pop_lsb(PromotionsBitboard);

			GeneratePromotions<Color, Type>(GamePosition, PawnSquare, TargetSquare, OutMoves);
		}
	}
}

template<PieceColor Color, PieceType Type>
void GenerateMoves(const Position& GamePosition, uint64_t LegalMovesBitboard, std::vector<SpecialMove>* OutMoves)
{
	const uint64_t OpponentOccupiedBitboard = GamePosition.Bitboards.Occupied[~Color];

	uint64_t PiecesBitboard = GamePosition.Bitboards.Pieces[Color][Type];

	while (PiecesBitboard)
	{
		int OldSquare = pop_lsb(PiecesBitboard);

		uint64_t AttacksBitboard =
			GamePosition.Attacks.GetPseudoAttacks(OldSquare, Type, Color, GamePosition.Bitboards.AllOccupied) & LegalMovesBitboard;

		uint64_t CapturesBitboard = AttacksBitboard & OpponentOccupiedBitboard;

		InsertCaptures(OldSquare, CapturesBitboard, OutMoves);

		uint64_t MovesBitboard = AttacksBitboard & ~CapturesBitboard;

		InsertMoves(OldSquare, MovesBitboard, OutMoves);
	}
}

template<GenType Type, PieceColor Color>
void GenerateAll(const Position& GamePosition, std::vector<SpecialMove>* OutMoves)
{
	int KingSquare = GamePosition.GetKingSquare(Color);
	uint64_t Checkers = GamePosition.CheckingPieces;
	uint64_t LegalMovesBitboard = ~GamePosition.Bitboards.GetPieces(King);

	if (Type != Evasions || !AreMultipleBitsSet(Checkers))
	{
		int CheckingSquare = lsb(Checkers);

		LegalMovesBitboard &=
				Type == Evasions ?		GamePosition.Attacks.SquaresBetween[KingSquare][CheckingSquare]
			:	Type == NonEvasions ?	~GamePosition.Bitboards.Occupied[Color]
			:	Type == CapturesOnly ?	GamePosition.Bitboards.Occupied[~Color]
			:/* Type == Quiet */		~GamePosition.Bitboards.AllOccupied;

		GeneratePawnMoves<Color, Type>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Knight>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Bishop>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Rook>(GamePosition, LegalMovesBitboard, OutMoves);
		GenerateMoves<Color, Queen>(GamePosition, LegalMovesBitboard, OutMoves);
	}

	uint64_t KingLegalMoves =
		Type == Evasions ?
		~GamePosition.Bitboards.Occupied[Color] :
		LegalMovesBitboard;

	uint64_t KingAttacksBitboard =
		GamePosition.Attacks.GetPseudoAttacks(KingSquare, King) & KingLegalMoves;

	uint64_t KingCapturesBitboard = KingAttacksBitboard & GamePosition.Bitboards.Occupied[~Color];
	uint64_t KingMovesBitboard = KingAttacksBitboard & ~KingCapturesBitboard;

	InsertCaptures(KingSquare, KingCapturesBitboard, OutMoves);
	InsertMoves(KingSquare, KingMovesBitboard, OutMoves);

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
	uint64_t PinnedBitboard = GamePosition.PinnedPieces[Color] & GamePosition.Bitboards.Occupied[Color];

	if (Checkers != 0)
	{
		GenerateMoves<Evasions>(GamePosition, Color, OutMoves);
	}
	else
	{
		GenerateMoves<NonEvasions>(GamePosition, Color, OutMoves);
	}

	for (size_t i = 0; i < OutMoves->size();)
	{
		const SpecialMove& CurrentMove = (*OutMoves)[i];

		if ((
			  IsBitSet(PinnedBitboard, CurrentMove.Move.OldSquare)
			  || CurrentMove.Move.OldSquare == KingSquare
			  || CurrentMove.ExtraInfo.bIsEnpassant)
			&& !GamePosition.IsLegalMove(CurrentMove))
		{
			(*OutMoves)[i] = std::move(OutMoves->back());
			OutMoves->pop_back();
		}
		else
		{
			i++;
		}
	}
}
