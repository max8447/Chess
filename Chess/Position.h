#pragma once

#include "Includes.h"

#include "State.h"
#include "Bitboard.h"
#include "Attacks.h"

struct Position
{
	State GameState;
	mutable Bitboard Bitboards;
	Attacks Attacks;

public:

	uint64_t CastlingPath[CastlingRights_Count];
	int8_t CastlingRookSquares[CastlingRights_Count]; // TODO: use

public:

	// only for use in move generation!

	mutable uint64_t PinningPieces[PieceColor_Count];	// mutable for now
	mutable uint64_t PinnedPieces[PieceColor_Count];	// mutable for now

	mutable uint64_t CheckingPieces;

public:

	const bool IsInCheck(PieceColor Color) const;
	const bool IsAttacked(int AttackedSquare, PieceColor AttackerColor) const;
	const uint64_t GetAttackers(int AttackedSquare) const;

	const bool CanCastle(CastlingRights CastlingRight) const
	{
		return GameState.AvailableCastlingRights & CastlingRight;
	}

	const bool IsCastlingBlocked(CastlingRights CastlingRight) const
	{
		return CastlingPath[CastlingRight] & Bitboards.AllOccupied;
	}

	const int GetKingSquare(PieceColor Color) const
	{
		return lsb(Bitboards.Pieces[Color][King]); // there is only ever one king per color
	}

private:

	void UpdatePins(PieceColor Color) const;

public:

	void AddCastlingRight(PieceColor Color, int RookOldSquare);
	void CalculateCheckData(PieceColor Color) const;

	bool IsLegalMove(const SpecialMove& Move) const;
};
