#include "Bitboard.h"

void Bitboard::Init(const std::vector<std::unique_ptr<Piece>>& BoardPieces)
{
	memset(this, 0, sizeof(*this));

	for (const auto& Piece : BoardPieces)
	{
		SetBit(Pieces[Piece->Color][Piece->Type], Piece->Square);
		SetBit(Occupied[Piece->Color], Piece->Square);
		SetBit(AllOccupied, Piece->Square);
	}

#ifdef _DEBUG
	ASSERT((Occupied[White] & Occupied[Black]) == 0);
	ASSERT((Occupied[White] | Occupied[Black]) == AllOccupied);
#endif
}

void Bitboard::MoveTo(PieceColor Color, PieceType Type, int OldSquare, int NewSquare)
{
#ifdef _DEBUG
	ASSERT((unsigned)OldSquare <= 63 && (unsigned)NewSquare <= 63);
#endif

	uint64_t FromBit = 1ull << OldSquare;
	uint64_t ToBit = 1ull << NewSquare;

	uint64_t MoveBit = FromBit | ToBit;

	Pieces[Color][Type] ^= MoveBit;
	Occupied[Color] ^= MoveBit;
	AllOccupied ^= MoveBit;
}
