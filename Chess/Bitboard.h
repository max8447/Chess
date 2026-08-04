#pragma once

#include "Includes.h"

#include "Piece.h"
#include "Attacks.h"

struct Bitboard
{
	uint64_t Pieces[PieceColor_Count][PieceType_Count];

	uint64_t Occupied[PieceColor_Count];
	uint64_t AllOccupied;

	void MoveTo(PieceColor Color, PieceType Type, int OldSquare, int NewSquare);

	template<typename... PieceTypes>
	uint64_t GetPieces(PieceColor Color, PieceTypes... Types) const
	{
		return (Pieces[Color][Types] | ...);
	}

	template<typename... PieceTypes>
	uint64_t GetPieces(PieceTypes... Types) const
	{
		return GetPieces<PieceTypes...>(White, Types...)
			|  GetPieces<PieceTypes...>(Black, Types...);
	}

	uint64_t GetAllPieces() const
	{
		return GetPieces(King, Queen, Bishop, Knight, Rook, Pawn);
	}
};

static inline constexpr uint64_t ToBitboard(int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63, false);
#endif

	return 1ull << Square;
}

static inline constexpr bool AreMultipleBitsSet(const uint64_t Bitboard)
{
	return Bitboard & (Bitboard - 1);
}

static inline constexpr bool IsBitSet(const uint64_t Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63, false);
#endif

	return (Bitboard & ToBitboard(Square)) != 0;
}

static inline constexpr void SetBit(uint64_t& Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63);
	ASSERT(!IsBitSet(Bitboard, Square));
#endif

	Bitboard |= ToBitboard(Square);
}

static inline constexpr void ClearBit(uint64_t& Bitboard, int Square)
{
#ifdef _DEBUG
	ASSERT((uint8_t)Square <= 63);
	ASSERT(IsBitSet(Bitboard, Square));
#endif

	Bitboard &= ~ToBitboard(Square);
}

static char* BitboardToString(const uint64_t Bitboard)
{
	constexpr int Bits = sizeof(Bitboard) * 8 - 1;

	char* Out = new char[(Bits + 10) * 2];
	int Idx = 0;

	Out[Idx++] = '\n';

	for (int i = Bits; i >= 0; i--)
	{
		auto [Rank, File] = Piece::SquareToRankFile(i);
		File = 7 - File;

		const int SquareIdx = Piece::RankFileToSquare({ Rank, File });

		Out[Idx++] = ((Bitboard >> SquareIdx) & 1) + '0';

		if (i % 8 == 0)
		{
			Out[Idx++] = '\n';
		}
		else
		{
			Out[Idx++] = ' ';
		}
	}

	Out[Idx] = '\0';

	return Out;
}
