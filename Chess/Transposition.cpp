#include "Transposition.h"

uint64_t Seed = 0xCAFEBABE12345678ull;

static uint64_t Rand64()
{
	uint64_t z = (Seed += 0x9E3779B97F4A7C15ull);

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;

	return z ^ (z >> 31);
}

TranspositionTable::TranspositionTable(int MegaBytes)
{
	size_t Bytes = 1'000'000ull * MegaBytes;

	Count = Bytes / sizeof(*Entries);

	Entries = new ZobrishEntry[Count];

	Init();
}

TranspositionTable::~TranspositionTable()
{
	Count = 0;

	delete Entries;
	Entries = nullptr;
}

void TranspositionTable::Init()
{
	for (int Piece = 0; Piece < ARRAY_LEN(PieceKeys); Piece++)
	{
		for (int Square = 0; Square < ARRAY_LEN(PieceKeys[Piece]); Square++)
		{
			PieceKeys[Piece][Square] = Rand64();
		}
	}

	SideToMoveKey = Rand64();

	for (int i = 0; i < ARRAY_LEN(CastlingKeys); i++)
	{
		CastlingKeys[i] = Rand64();
	}

	for (int i = 0; i < ARRAY_LEN(EnpassantKeys); i++)
	{
		EnpassantKeys[i] = Rand64();
	}
}

ZobrishEntry* TranspositionTable::GetEntry(uint64_t Hash) const
{
	size_t Idx = Hash & (Count - 1);

	ZobrishEntry& Entry = Entries[Idx];

	if (Entry.HashValue == Hash)
	{
		return &Entry;
	}

	return nullptr;
}

void TranspositionTable::Store(uint64_t Hash, int Depth, int Score, HashFlag Flag)
{
	size_t Idx = Hash & (Count - 1);

	ZobrishEntry& Entry = Entries[Idx];

	if (Depth >= Entry.Depth)
	{
		Entry.HashValue = Hash;
		Entry.Depth = Depth;
		Entry.Score = Score;
		Entry.Flag = Flag;
	}
}
