#pragma once

#include "Includes.h"

#include "Piece.h"
#include "CastlingRights.h"

struct State
{
	PieceColor CurrentMove = White; // white starts by default
	mutable CastlingRights AvailableCastlingRights = NoCastling;	// mutable for now
	mutable int8_t EnpassantSquare = -1;							// mutable for now
	int PawnPromotionSquare = -1;
	int HalfMoveClock = 0;
	int FullMoveCounter = 0;
	bool bGameEnded = false;
};
