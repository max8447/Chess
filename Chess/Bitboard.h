#pragma once

#include "Includes.h"

#include "Piece.h"

struct Bitboard
{
	uint64_t Pieces[PieceColor_Count][PieceType_Count];

	uint64_t Occupied[PieceColor_Count];
	uint64_t AllOccupied;

	void Init(const std::vector<std::unique_ptr<Piece>>& BoardPieces);

	void MoveTo(PieceColor Color, PieceType Type, int OldSquare, int NewSquare);
};

static inline bool IsBitSet(const uint64_t Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63, false);
#endif

	return (Bitboard & (1ull << Square)) != 0;
}

static inline void SetBit(uint64_t& Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63);
	ASSERT(!IsBitSet(Bitboard, Square));
#endif

	Bitboard |= 1ull << Square;
}

static inline void ClearBit(uint64_t& Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63);
	ASSERT(IsBitSet(Bitboard, Square));
#endif

	Bitboard &= ~(1ull << Square);
}

static char* BitboardToString(const uint64_t Bitboard)
{
	constexpr int Bits = sizeof(Bitboard) * 8 - 1;

	char* Out = new char[Bits + 10];
	int Idx = 0;

	for (int i = Bits; i >= 0; i--)
	{
		Out[Idx++] = ((Bitboard >> i) & 1) + '0';

		if (i % 8 == 0)
		{
			Out[Idx++] = ' ';
		}
	}

	Out[Idx] = '\0';

	return Out;
}
