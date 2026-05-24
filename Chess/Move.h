#pragma once

#include "Includes.h"

#include "Piece.h"

struct SimpleMove
{
	int8_t OldSquare;
	int8_t NewSquare;

	constexpr SimpleMove()
		: OldSquare((int8_t)-1), NewSquare((int8_t)-1)
	{}

	constexpr SimpleMove(int InOldSquare, int InNewSquare)
		: OldSquare((int8_t)InOldSquare), NewSquare((int8_t)InNewSquare)
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
		OldSquare = -1;
		NewSquare = -1;
	}
};

enum SpecialMoveType : uint8_t
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

	union
	{
		int8_t EnpassantSquare;
		PieceType PromotedPieceType;
		int8_t bIsEnpassant; // -1 = false, everything else = true
	} ExtraInfo{ -1 };

	Piece* OtherPiece = nullptr;

	constexpr SpecialMove()
		: Move(), Type(None), OtherPieceMove()
	{}

	constexpr SpecialMove(SimpleMove&& InMove)
		: Move(InMove), Type(None), OtherPieceMove()
	{}

	constexpr SpecialMove(SimpleMove InMove, SpecialMoveType InType, SimpleMove InOtherPieceMove)
		: Move(InMove), Type(InType), OtherPieceMove(InOtherPieceMove)
	{}

	static inline size_t CopyConstructorCalls = 0;
	static inline size_t MoveConstructorCalls = 0;
	static inline size_t CopyAssignmentCalls = 0;
	static inline size_t MoveAssignmentCalls = 0;

	//SpecialMove(const SpecialMove& Other)
	//{
	//	CopyConstructorCalls++;
	//	memcpy(this, &Other, sizeof(*this));
	//}

	//SpecialMove(SpecialMove&& Other) noexcept
	//{
	//	MoveConstructorCalls++;
	//	memcpy(this, &Other, sizeof(*this));
	//}

	//SpecialMove& operator=(const SpecialMove& Other)
	//{
	//	CopyAssignmentCalls++;
	//	memcpy(this, &Other, sizeof(*this));
	//	return *this;
	//}

	//SpecialMove& operator=(SpecialMove&& Other) noexcept
	//{
	//	MoveAssignmentCalls++;
	//	memcpy(this, &Other, sizeof(*this));
	//	return *this;
	//}

	static void ResetCounters()
	{
		CopyConstructorCalls = 0;
		MoveConstructorCalls = 0;
		CopyAssignmentCalls = 0;
		MoveAssignmentCalls = 0;
	}

	const bool IsValid() const
	{
		bool bIsValid = Move.IsValid();

		if (Type & Castle)
		{
			bIsValid = bIsValid && OtherPieceMove.IsValid();
		}

		if (Type & Capture)
		{
			bIsValid = bIsValid && OtherPieceMove.OldSquare != -1 && OtherPieceMove.NewSquare == -1 && OtherPiece;
		}

		if (Type & PawnPromotion)
		{
			bIsValid = bIsValid /* && ExtraInfo.PromotedPieceType != Null */;
		}

		if (Type & PawnDoublePush)
		{
			bIsValid = bIsValid && ExtraInfo.EnpassantSquare != -1;
		}

		return bIsValid;
	}

	const bool IsZero() const // old square is the same as new square and type is None
	{
		return Move.IsZero() && Type == None;
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