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

	SimpleMove(const char* AlgebraicOldSquare, const char* AlgebraicNewSquare) // no safety guaranteed
	{
		std::array<char, 2> OldSquareArray = { AlgebraicOldSquare[0], AlgebraicOldSquare[1] };
		std::array<char, 2> NewSquareArray = { AlgebraicNewSquare[0], AlgebraicNewSquare[1] };

		OldSquare = Piece::RankFileToSquare(Piece::AlgebraicToRankFile(OldSquareArray));
		NewSquare = Piece::RankFileToSquare(Piece::AlgebraicToRankFile(NewSquareArray));
	}

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
	None,
	PawnPromotion,
	PossibleEnpassant,
	Capture,
	Castle,
};

struct SpecialMove
{
	SimpleMove Move;

	// if it's a capture then Move will be the capturing piece's move and OtherPieceMove will be the captured piece's move with NewSquare set to -1
	// if it's a castle then Move will be the king's move and OtherPieceMove will be the rook's move

	SpecialMoveType Type;
	SimpleMove OtherPieceMove;

	Piece* MovedPiece;
	Piece* CapturedPiece;
	Piece* CastledRook;

	SpecialMove()
		: Move(), Type(None), OtherPieceMove(), MovedPiece(nullptr), CapturedPiece(nullptr), CastledRook(nullptr)
	{}

	SpecialMove(SimpleMove InMove, SpecialMoveType InType, SimpleMove InOtherPieceMove)
		: Move(InMove), Type(InType), OtherPieceMove(InOtherPieceMove), MovedPiece(nullptr), CapturedPiece(nullptr), CastledRook(nullptr)
	{}

	const bool IsValid() const // invalid moves (squares are -1)
	{
		return Move.OldSquare != -1 && Move.NewSquare != -1 &&
			(Type == None || Type == PawnPromotion ?
				true :
				Type == Capture ?
				OtherPieceMove.OldSquare != -1 && OtherPieceMove.NewSquare == -1 :
				OtherPieceMove.IsValid());
	}

	const bool IsZero() const // old square and new square are the same
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