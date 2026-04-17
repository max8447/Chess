#pragma once

#include "Includes.h"

enum PieceType
{
	King = 0,
	Queen = 1,
	Bishop = 2,
	Knight = 3,
	Rook = 4,
	Pawn = 5
};

enum PieceColor
{
	White,
	Black
};

struct Piece
{
	int Square;
	PieceType Type;
	PieceColor Color;

	Piece() = delete;

	Piece(int InSquare, PieceType InType, PieceColor InColor)
		: Square(InSquare)
		, Type(InType)
		, Color(InColor)
	{
	}

	bool IsAllowedMove(int NewSquare) const;

	static constexpr std::array<int, 9> GetAvailableMoves(PieceType Type);

	static constexpr std::pair<int, int> SquareToRankFile(int Square);
	static constexpr int RankFileToSquare(int Rank, int File);

	static constexpr std::pair<int, int> RotateCW(std::pair<int, int> RankFile);
	static constexpr std::pair<int, int> RotateCCW(std::pair<int, int> RankFile);

	static_assert(std::is_same_v<decltype(RotateCW), decltype(RotateCCW)>, "RotateCW and RotateCCW must always have the same signature!");
};

constexpr std::pair<int, int> Piece::SquareToRankFile(int Square)
{
	int Rank = Square / 8;
	int File = Square % 8;

	return RotateCW({ Rank, File });
}

constexpr int Piece::RankFileToSquare(int Rank, int File)
{
	const auto [RotatedRank, RotatedFile] = RotateCCW({ Rank, File });

	int Square = RotatedRank * 8 + RotatedFile;

#ifdef _DEBUG
	const auto [RankTmp, FileTmp] = SquareToRankFile(Square);

	IM_ASSERT(RankTmp == Rank && FileTmp == File); // check unrotated values since we rotate them back in SquareToRankFile
#endif

	return Square;
}

constexpr std::pair<int, int> Piece::RotateCW(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return std::pair<int, int>(File, 7 - Rank);
}

constexpr std::pair<int, int> Piece::RotateCCW(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return std::pair<int, int>(7 - File, Rank);
}
