#pragma once

#include "Includes.h"

#include "Move.h"

namespace StaticCastlingRights
{
	constexpr SpecialMove KingSide[PieceColor_Count] = {
		SpecialMove{ { e1, g1 }, Castle, { h1, f1 } },	// white
		SpecialMove{ { e8, g8 }, Castle, { h8, f8 } },	// black
	};
	constexpr SpecialMove QueenSide[PieceColor_Count] = {
		SpecialMove{ { e1, c1 }, Castle, { a1, d1 } },	// white
		SpecialMove{ { e8, c8 }, Castle, { a8, d8 } },	// black
	};
};

enum CastlingRights : uint8_t
{
	WhiteKingSide = 1 << 0,
	WhiteQueenSide = 1 << 1,
	BlackKingSide = 1 << 2,
	BlackQueenSide = 1 << 3,

	KingSide = WhiteKingSide | BlackKingSide,
	QueenSide = WhiteQueenSide | BlackQueenSide,
	WhiteCastling = WhiteKingSide | WhiteQueenSide,
	BlackCastling = BlackKingSide | BlackQueenSide,

	NoCastling = 0,
	AnyCastling = WhiteCastling | BlackCastling,
};

ENUM_OPERATORS(CastlingRights);