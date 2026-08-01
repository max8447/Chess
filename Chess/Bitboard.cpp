#include "Bitboard.h"

void Bitboard::MoveTo(PieceColor Color, PieceType Type, int OldSquare, int NewSquare)
{
#ifdef _DEBUG
	ASSERT((unsigned)OldSquare <= 63 && (unsigned)NewSquare <= 63);
	ASSERT((Occupied[Black] | Occupied[White]) == AllOccupied);
	ASSERT((Occupied[Black] & Occupied[White]) == 0);
#endif

	uint64_t FromBit = ToBitboard(OldSquare);
	uint64_t ToBit = ToBitboard(NewSquare);

	uint64_t MoveBit = FromBit | ToBit;

	Pieces[Color][Type] ^= MoveBit;
	Occupied[Color] ^= MoveBit;
	AllOccupied ^= MoveBit;

#ifdef _DEBUG
	ASSERT((Occupied[Black] | Occupied[White]) == AllOccupied);
	ASSERT((Occupied[Black] & Occupied[White]) == 0);
#endif // _DEBUG
}
