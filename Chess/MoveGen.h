#pragma once

#include "Includes.h"

#include "Position.h"

enum GenType
{
	Legal,			// all legal moves
	Quiet,			// all non-capture, non-promotion, non-castle moves
	CapturesOnly,	// all capture-only moves
	Evasions,		// all moves which escape check
	NonEvasions,	// all moves which don't escape check
};

template<GenType Type>
void GenerateMoves(const Position& GamePosition, PieceColor Color, std::vector<SpecialMove>* OutMoves);

template<GenType Type>
void GenerateMoves(const Position& GamePosition, std::vector<SpecialMove>* OutMoves)
{
	return GenerateMoves<Type>(GamePosition, GamePosition.GameState.CurrentMove, OutMoves);
}
