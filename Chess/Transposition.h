#pragma once

#include "Includes.h"

#include "Piece.h"

enum HashFlag
{
	EXACT,
	LOWERBOUND,
	UPPERBOUND,
};

struct ZobrishEntry
{
	uint64_t HashValue;

	int Depth;
	int Score;

	HashFlag Flag;
};

struct TranspositionTable
{
	ZobrishEntry* Entries;
	size_t Count;

	uint64_t PieceKeys[PieceType_Count * PieceColor_Count][AlgebraicSquare_Count];
	uint64_t SideToMoveKey;
	uint64_t CastlingKeys[16];
	uint64_t EnpassantKeys[AlgebraicSquare_Count];

	TranspositionTable(int MegaBytes);
	~TranspositionTable();

	void Init();

	ZobrishEntry* GetEntry(uint64_t Hash) const;
	void Store(uint64_t Hash, int Depth, int Score, HashFlag Flag);
};