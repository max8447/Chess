#include "Position.h"

const bool Position::IsInCheck(PieceColor Color) const
{
	int KingSquare = GetKingSquare(Color);
	PieceColor AttackerColor = ~Color;

	return IsAttacked(KingSquare, AttackerColor);
}

const bool Position::IsAttacked(int AttackedSquare, PieceColor AttackerColor) const
{
	ASSERT((uint8_t)AttackedSquare <= 63, false);

	const auto [Rank, File] = Piece::SquareToRankFile(AttackedSquare);

	if (Bitboards.Pieces[AttackerColor][Pawn] & Attacks.PawnAttacks[~AttackerColor][AttackedSquare])
	{
		return true;
	}

	if (Bitboards.Pieces[AttackerColor][Knight] & Attacks.KnightAttacks[AttackedSquare])
	{
		return true;
	}

	if (Bitboards.Pieces[AttackerColor][King] & Attacks.KingAttacks[AttackedSquare])
	{
		return true;
	}

	if (Attacks.IsPieceAttacking(AttackedSquare, AttackerColor, Queen, Bitboards.Pieces[AttackerColor][Queen], Bitboards.AllOccupied, Bitboards.Occupied[AttackerColor]))
	{
		return true;
	}

	if (Attacks.IsPieceAttacking(AttackedSquare, AttackerColor, Bishop, Bitboards.Pieces[AttackerColor][Bishop], Bitboards.AllOccupied, Bitboards.Occupied[AttackerColor]))
	{
		return true;
	}

	if (Attacks.IsPieceAttacking(AttackedSquare, AttackerColor, Rook, Bitboards.Pieces[AttackerColor][Rook], Bitboards.AllOccupied, Bitboards.Occupied[AttackerColor]))
	{
		return true;
	}

	return false;
}

const uint64_t Position::GetAttackers(int AttackedSquare) const
{
	return (Attacks.GetPseudoAttacks(AttackedSquare, Pawn, White) & Bitboards.Pieces[Black][Pawn])
		|  (Attacks.GetPseudoAttacks(AttackedSquare, Pawn, Black) & Bitboards.Pieces[White][Pawn])
		|  (Attacks.GetPseudoAttacks(AttackedSquare, Knight) & Bitboards.GetPieces(Knight))
		|  (Attacks.GetBishopMoves(AttackedSquare, Bitboards.AllOccupied, 0) & Bitboards.GetPieces(Bishop, Queen))
		|  (Attacks.GetRookMoves(AttackedSquare, Bitboards.AllOccupied, 0) & Bitboards.GetPieces(Rook, Queen))
		|  (Attacks.GetPseudoAttacks(AttackedSquare, King) & Bitboards.GetPieces(King));
}

void Position::UpdatePins(PieceColor Color) const
{
	PinningPieces[~Color] = 0;
	PinnedPieces[Color] = 0;

	int KingSquare = GetKingSquare(Color);

	uint64_t Pinners =
		((Attacks.GetPseudoAttacks(KingSquare, Rook) & Bitboards.GetPieces(Queen, Rook))
		|(Attacks.GetPseudoAttacks(KingSquare, Bishop) & Bitboards.GetPieces(Queen, Bishop)))
		& Bitboards.Occupied[~Color];

	uint64_t Occupancy = Bitboards.AllOccupied ^ Pinners;

	while (Pinners)
	{
		int PinnerSquare = pop_lsb(Pinners);

		uint64_t Bitboard = Attacks.SquaresBetween[KingSquare][PinnerSquare] & Occupancy;

		if (Bitboard != 0 && !AreMultipleBitsSet(Bitboard))
		{
			PinnedPieces[Color] |= Bitboard;

			if (Bitboard & Bitboards.Occupied[~Color])
			{
				PinningPieces[~Color] |= ToBitboard(PinnerSquare);
			}
		}
	}
}

void Position::AddCastlingRight(PieceColor Color, int RookOldSquare)
{
	int KingOldSquare = GetKingSquare(Color);

	ASSERT((uint8_t)KingOldSquare <= 63);

	CastlingRights CastlingRight = Color & (KingOldSquare < RookOldSquare ? KingSide : QueenSide);

	int KingNewSquare = CastlingRight & KingSide ? StaticCastlingRights::KingSide[Color].Move.NewSquare : StaticCastlingRights::QueenSide[Color].Move.NewSquare;
	int RookNewSquare = CastlingRight & KingSide ? StaticCastlingRights::KingSide[Color].OtherPieceMove.NewSquare : StaticCastlingRights::QueenSide[Color].OtherPieceMove.NewSquare;

	GameState.AvailableCastlingRights |= CastlingRight;
	CastlingRookSquares[CastlingRight] = RookOldSquare;

	CastlingPath[CastlingRight] =
		(Attacks.SquaresBetween[RookOldSquare][RookNewSquare] | Attacks.SquaresBetween[KingOldSquare][KingNewSquare]) & ~(ToBitboard(KingOldSquare) | ToBitboard(RookOldSquare));
}

void Position::CalculateCheckData(PieceColor Color) const
{
	// update both, no matter the color

	UpdatePins(White);
	UpdatePins(Black);
	
	ASSERT((Bitboards.Occupied[Black] | Bitboards.Occupied[White]) == Bitboards.AllOccupied);
	ASSERT((Bitboards.Occupied[Black] & Bitboards.Occupied[White]) == 0);

	int KingSquare = GetKingSquare(Color);

	CheckingPieces = GetAttackers(KingSquare) & Bitboards.Occupied[~Color];
}
