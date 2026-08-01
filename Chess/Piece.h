#pragma once

#include "Includes.h"

enum PieceType : int8_t
{
	Null = -1,
	King = 0,
	Queen = 1,
	Bishop = 2,
	Knight = 3,
	Rook = 4,
	Pawn = 5,

	PieceType_Count = 6,
};

constexpr int PieceValues[] = { // has to match ordering of enum above
	INT32_MAX,	// king
	9,			// queen
	3,			// bishop
	3,			// knight
	5,			// rook
	1,			// pawn
};

enum PieceColor : uint8_t
{
	White,
	Black,

	PieceColor_Count = 2,
};

constexpr PieceColor operator~(PieceColor Color) // flip color
{
	return (PieceColor)(1 - Color);
}

enum AlgebraicSquare : uint8_t
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8,

	AlgebraicSquare_Count = 64,
};

enum Direction : int8_t
{
	North		=  8,
	South		= -8,
	East		=  1,
	West		= -1,
	NorthEast	= North + East,
	NorthWest	= North + West,
	SouthEast	= South + East,
	SouthWest	= South + West,
};

constexpr Direction operator-(Direction Dir)
{
	return (Direction)(-(int)Dir);
}

struct Piece
{
	int Square;
	PieceType Type;
	PieceColor Color;

	Piece() = delete;

	constexpr Piece(int InSquare, PieceType InType, PieceColor InColor)
		: Square(InSquare)
		, Type(InType)
		, Color(InColor)
	{
	}

	constexpr Piece(const Piece& InPiece)
		: Square(InPiece.Square)
		, Type(InPiece.Type)
		, Color(InPiece.Color)
	{
	}

	bool IsAllowedMove(int NewSquare) const;

	static constexpr std::array<int, 9> GetAvailableMoves(PieceType Type);

	static constexpr std::pair<int, int> SquareToRankFile(int Square);

	static constexpr int8_t RankFileToSquare(int Rank, int File);
	static constexpr int8_t RankFileToSquare(std::pair<int, int> RankFile);

	static constexpr std::array<char, 3> SquareToAlgebraic(int Square);
	static constexpr std::array<char, 3> RankFileToAlgebraic(std::pair<int, int> RankFile);

	static constexpr std::pair<int, int> AlgebraicToRankFile(const std::array<char, 2>& Algebraic); // no safety guaranteed

	static constexpr std::pair<int, int> RotateCW(std::pair<int, int> RankFile);	// only use in graphic contexts
	static constexpr std::pair<int, int> RotateCCW(std::pair<int, int> RankFile);	// only use in graphic contexts

	static constexpr int8_t RotateCW(int Square);	// only use in graphic contexts
	static constexpr int8_t RotateCCW(int Square);	// only use in graphic contexts

	static constexpr const char* ColorToString(PieceColor Color);
};

constexpr std::pair<int, int> Piece::SquareToRankFile(int Square)
{
	int Rank = Square / 8;
	int File = Square % 8;

	return std::pair{ Rank, File };
}

constexpr int8_t Piece::RankFileToSquare(int Rank, int File)
{
	int Square = Rank * 8 + File;

	return Square;
}

constexpr int8_t Piece::RankFileToSquare(std::pair<int, int> RankFile)
{
	const auto [Rank, File] = RankFile;

	return RankFileToSquare(Rank, File);
}

constexpr std::array<char, 3> Piece::SquareToAlgebraic(int Square)
{
	return RankFileToAlgebraic(SquareToRankFile(Square));
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

constexpr std::pair<int, int> Piece::AlgebraicToRankFile(const std::array<char, 2>& Algebraic)
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

constexpr int8_t Piece::RotateCW(int Square)
{
	const auto [Rank, File] = RotateCW(SquareToRankFile(Square));

	return RankFileToSquare(Rank, File);
}

constexpr int8_t Piece::RotateCCW(int Square)
{
	const auto [Rank, File] = RotateCCW(SquareToRankFile(Square));

	return RankFileToSquare(Rank, File);
}

constexpr const char* Piece::ColorToString(PieceColor Color)
{
	return Color == White ? "WHITE" : "BLACK";
}
