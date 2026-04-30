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

enum AlgebraicSquare
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
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

	Piece(const Piece& InPiece)
		: Square(InPiece.Square)
		, Type(InPiece.Type)
		, Color(InPiece.Color)
	{
	}

	int GetDirection() const;
	std::pair<int, int> GetRankFile(int Square = -1) const;
	int GetSquare(int Rank = -1, int File = -1) const;

	bool IsAllowedMove(int NewSquare) const;

	static constexpr std::array<int, 9> GetAvailableMoves(PieceType Type);

	static constexpr std::pair<int, int> SquareToRankFile(int Square);

	static constexpr int RankFileToSquare(int Rank, int File);
	static constexpr int RankFileToSquare(std::pair<int, int> RankFile);

	static constexpr std::array<char, 3> RankFileToAlgebraic(std::pair<int, int> RankFile);
	static constexpr std::pair<int, int> AlgebraicToRankFile(std::array<char, 2> Algebraic); // no safety guaranteed

	static constexpr std::pair<int, int> RotateCW(std::pair<int, int> RankFile);	// only use in graphic contexts
	static constexpr std::pair<int, int> RotateCCW(std::pair<int, int> RankFile);	// only use in graphic contexts

	static constexpr int RotateCW(int Square);	// only use in graphic contexts
	static constexpr int RotateCCW(int Square);	// only use in graphic contexts
};

constexpr std::pair<int, int> Piece::SquareToRankFile(int Square)
{
	int Rank = Square / 8;
	int File = Square % 8;

	return std::pair{ Rank, File };
}

constexpr int Piece::RankFileToSquare(int Rank, int File)
{
	int Square = Rank * 8 + File;

	return Square;
}

constexpr int Piece::RankFileToSquare(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return RankFileToSquare(Rank, File);
}

constexpr std::array<char, 3> Piece::RankFileToAlgebraic(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return {
		(char)(Rank + 'a'),
		(char)(File + '1'),
		'\0',
	};
}

constexpr std::pair<int, int> Piece::AlgebraicToRankFile(std::array<char, 2> Algebraic)
{
	return std::pair{ Algebraic[0] - 'a', Algebraic[1] - '1' };
}

constexpr std::pair<int, int> Piece::RotateCW(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return { 7 - File, Rank };
}

constexpr std::pair<int, int> Piece::RotateCCW(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return { File, 7 - Rank };
}

constexpr int Piece::RotateCW(int Square)
{
	const auto [Rank, File] = RotateCW(SquareToRankFile(Square));

	return RankFileToSquare(Rank, File);
}

constexpr int Piece::RotateCCW(int Square)
{
	const auto [Rank, File] = RotateCCW(SquareToRankFile(Square));

	return RankFileToSquare(Rank, File);
}
