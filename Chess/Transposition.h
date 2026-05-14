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

	uint64_t PieceKeys[PieceType_Count * 2][64];
	uint64_t SideToMoveKey;
	uint64_t CastlingKeys[16];
	uint64_t EnpassantKeys[64];

	TranspositionTable(int MegaBytes);
	~TranspositionTable();

	void Init();

	ZobrishEntry* GetEntry(uint64_t Hash) const;
	void Store(uint64_t Hash, int Depth, int Score, HashFlag Flag);
};