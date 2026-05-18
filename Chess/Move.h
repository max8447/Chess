#pragma once

#include "Includes.h"

#include "Piece.h"

struct SimpleMove
{
	int OldSquare;
	int NewSquare;

	SimpleMove()
		: OldSquare(-1), NewSquare(-1)
	{}

	SimpleMove(int InOldSquare, int InNewSquare)
		: OldSquare(InOldSquare), NewSquare(InNewSquare)
	{}

	const bool IsValid() const // invalid moves (squares are -1)
	{
		return OldSquare != -1 && NewSquare != -1;
	}

	const bool IsZero() const // old square and new square are the same
	{
		return OldSquare == NewSquare;
	}

	const bool IsAllowed() const // is valid AND is NOT zero
	{
		return IsValid() && !IsZero();
	}

	void Invalidate() // set *only* NewSquare to -1
	{
		NewSquare = -1;
	}
};

enum SpecialMoveType
{
	None					= 0,
	Capture					= 1 << 1,
	Castle					= 1 << 2,
	PawnPromotion			= 1 << 3,
	PawnDoublePush			= 1 << 4,
};

ENUM_OPERATORS(SpecialMoveType);

struct SpecialMove
{
	SimpleMove Move;

	SpecialMoveType Type;
	SimpleMove OtherPieceMove;

	int EnpassantSquare = -1;

	Piece* MovedPiece = nullptr;
	Piece* CapturedPiece = nullptr;
	Piece* CastledRook = nullptr;

	PieceType PromotedPieceType = Null;
	bool bIsEnpassant = false;

	SpecialMove()
		: Move(), Type(None), OtherPieceMove()
	{}

	SpecialMove(SimpleMove InMove, SpecialMoveType InType, SimpleMove InOtherPieceMove)
		: Move(InMove), Type(InType), OtherPieceMove(InOtherPieceMove)
	{}

	const bool IsValid() const
	{
		bool bIsValid = Move.IsValid() && MovedPiece;

		if (Type & Castle)
		{
			bIsValid = bIsValid && OtherPieceMove.IsValid();
		}

		if (Type & Capture)
		{
			bIsValid = bIsValid && OtherPieceMove.OldSquare != -1 && OtherPieceMove.NewSquare == -1 && CapturedPiece;
		}

		if (Type & PawnPromotion)
		{
			bIsValid = bIsValid && MovedPiece->Type == Pawn /* && PromotedPieceType != Null */;
		}

		if (Type & PawnDoublePush)
		{
			bIsValid = bIsValid && EnpassantSquare != -1;
		}

		return bIsValid;
	}

	const bool IsZero() const // old square is the same as new square and type is None
	{
		return Move.OldSquare == Move.NewSquare && Type == None;
	}

	const bool IsAllowed() const // is valid AND is NOT zero
	{
		return IsValid() && !IsZero();
	}

	void Invalidate()
	{
		*this = SpecialMove{};
	}
};

struct ScoredMove
{
	SpecialMove Move;
	int Score;
};