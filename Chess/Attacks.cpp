#include "Attacks.h"

#include "Bitboard.h"

static constexpr uint64_t PawnMask(int Square, PieceColor Color)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Mask = 0;

	int Dir = (Color == White) ? 1 : -1;

	int Targets[2] = { -1, 1 };

	for (int i = 0; i < 2; i++)
	{
		int CurrentRank = Rank + Dir;
		int CurrentFile = File + Targets[i];

		if (CurrentRank < 0 || CurrentRank > 7 ||
			CurrentFile < 0 || CurrentFile > 7)
		{
			continue;
		}

		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	return Mask;
}

static constexpr uint64_t KnightMask(int Square)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Mask = 0;

	constexpr int DeltaRanks[8] = { 2, 2, -2, -2, 1, 1, -1, -1 };
	constexpr int DeltaFiles[8] = { 1, -1, 1, -1, 2, -2, 2, -2 };

	for (int i = 0; i < 8; i++)
	{
		int CurrentRank = Rank + DeltaRanks[i];
		int CurrentFile = File + DeltaFiles[i];

		if (CurrentRank < 0 || CurrentRank > 7 ||
			CurrentFile < 0 || CurrentFile > 7)
		{
			continue;
		}

		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	return Mask;
}

static constexpr uint64_t BishopMask(int Square)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Mask = 0;

	for (int CurrentRank = Rank + 1, CurrentFile = File + 1; CurrentRank <= 6 && CurrentFile <= 6; CurrentRank++, CurrentFile++)
	{
		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	for (int CurrentRank = Rank + 1, CurrentFile = File - 1; CurrentRank <= 6 && CurrentFile >= 1; CurrentRank++, CurrentFile--)
	{
		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	for (int CurrentRank = Rank - 1, CurrentFile = File + 1; CurrentRank >= 1 && CurrentFile <= 6; CurrentRank--, CurrentFile++)
	{
		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	for (int CurrentRank = Rank - 1, CurrentFile = File - 1; CurrentRank >= 1 && CurrentFile >= 1; CurrentRank--, CurrentFile--)
	{
		Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
	}

	return Mask;
}

static constexpr uint64_t RookMask(int Square)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Mask = 0;

	for (int CurrentRank = Rank + 1; CurrentRank <= 6; CurrentRank++) Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, File);
	for (int CurrentRank = Rank - 1; CurrentRank >= 1; CurrentRank--) Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, File);

	for (int CurrentRank = File + 1; CurrentRank <= 6; CurrentRank++) Mask |= 1ull << Piece::RankFileToSquare(Rank, CurrentRank);
	for (int CurrentRank = File - 1; CurrentRank >= 1; CurrentRank--) Mask |= 1ull << Piece::RankFileToSquare(Rank, CurrentRank);

	return Mask;
}

static constexpr uint64_t KingMask(int Square)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Mask = 0;

	for (int DeltaRank = -1; DeltaRank <= 1; DeltaRank++)
	{
		for (int DeltaFile = -1; DeltaFile <= 1; DeltaFile++)
		{
			if (DeltaRank == 0 && DeltaFile == 0)
			{
				continue;
			}

			int CurrentRank = Rank + DeltaRank;
			int CurrentFile = File + DeltaFile;

			if (CurrentRank < 0 || CurrentRank > 7 ||
				CurrentFile < 0 || CurrentFile > 7)
			{
				break;
			}

			Mask |= 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);
		}
	}

	return Mask;
}

static constexpr std::vector<uint64_t> CreateBlockerBitboards(uint64_t MovementMask)
{
	std::vector<int> MoveSquareIndicies;

	for (int i = 0; i < 64; i++)
	{
		if (((MovementMask >> i) & 1) == 1)
		{
			MoveSquareIndicies.push_back(i);
		}
	}

	int NumPatterns = 1ull << MoveSquareIndicies.size();
	std::vector<uint64_t> BlockerBitboards(NumPatterns);

	for (int PatternIdx = 0; PatternIdx < NumPatterns; PatternIdx++)
	{
		for (int BitIdx = 0; BitIdx < MoveSquareIndicies.size(); BitIdx++)
		{
			int Bit = (PatternIdx >> BitIdx) & 1;
			BlockerBitboards[PatternIdx] |= (uint64_t)Bit << MoveSquareIndicies[BitIdx];
		}
	}

	return BlockerBitboards;
}

static constexpr uint64_t CreateLegalMoveBitboard(PieceType Type, int Square, uint64_t BlockerBitboard)
{
	const auto [Rank, File] = Piece::SquareToRankFile(Square);

	uint64_t Bitboard = 0;

	constexpr int BishopDirRanks[4] = { 1,  1, -1, -1 };
	constexpr int BishopDirFiles[4] = { 1, -1,  1, -1 };

	constexpr int RookDirRanks[4] = { -1, 1, 0,  0 };
	constexpr int RookDirFiles[4] = { 0, 0, 1, -1 };

	const int* DirRanks = Type == Bishop ? BishopDirRanks : RookDirRanks;
	const int* DirFiles = Type == Bishop ? BishopDirFiles : RookDirFiles;

	for (int i = 0; i < 4; i++)
	{
		for (int Distance = 0; Distance < 8; Distance++)
		{
			int CurrentRank = Rank + DirRanks[i] * Distance;
			int CurrentFile = File + DirFiles[i] * Distance;

			if (CurrentRank < 0 || CurrentRank > 7 ||
				CurrentFile < 0 || CurrentFile > 7)
			{
				break;
			}

			uint64_t SquareBit = 1ull << Piece::RankFileToSquare(CurrentRank, CurrentFile);

			Bitboard |= SquareBit;

			if ((BlockerBitboard & SquareBit) != 0)
			{
				break;
			}
		}
	}

	return Bitboard;
}

static constexpr std::vector<uint64_t> CreateLookupTable(PieceType Type, int Square, uint64_t Magic, int LeftShift)
{
	int NumBits = 64 - LeftShift;
	int LookupSize = 1 << NumBits; // 2 ^ NumBits

	std::vector<uint64_t> Table(LookupSize);

	uint64_t MovementMask = Type == Bishop ? BishopMask(Square) : RookMask(Square);
	std::vector<uint64_t> BlockerBitboards = CreateBlockerBitboards(MovementMask);

	for (const uint64_t BlockerBitboard : BlockerBitboards)
	{
		uint64_t Idx = (BlockerBitboard * Magic) >> LeftShift;
		uint64_t LegalMoveBitboard = CreateLegalMoveBitboard(Type, Square, BlockerBitboard);

		Table[Idx] = LegalMoveBitboard;
	}

	return Table;
}

void Attacks::Init()
{
	for (int Square = 0; Square < AlgebraicSquare_Count; Square++)
	{
		// invert the colors since the attacks are for the opposite color

		PawnAttacks[White][Square] = PawnMask(Square, Black);
		PawnAttacks[Black][Square] = PawnMask(Square, White);

		KnightAttacks[Square] = KnightMask(Square);

		BishopMasks[Square] = BishopMask(Square);
		RookMasks[Square] = RookMask(Square);

		KingAttacks[Square] = KingMask(Square);
	}

	BishopMovesLookup.resize(AlgebraicSquare_Count);
	RookMovesLookup.resize(AlgebraicSquare_Count);

	for (int Square = 0; Square < AlgebraicSquare_Count; Square++)
	{
		BishopMovesLookup[Square] = CreateLookupTable(Bishop, Square, BishopMagics[Square], BishopShifts[Square]);
		RookMovesLookup[Square] = CreateLookupTable(Rook, Square, RookMagics[Square], RookShifts[Square]);
	}
}

uint64_t Attacks::GetBishopMoves(int Square, const uint64_t AllOccupiedBitboard, const uint64_t FriendlyPiecesBitboard) const
{
	uint64_t BlockerBitboard = AllOccupiedBitboard & BishopMasks[Square];
	uint64_t Key = (BlockerBitboard * BishopMagics[Square]) >> BishopShifts[Square];
	uint64_t AllMovesBitboard = BishopMovesLookup[Square][Key];

	return AllMovesBitboard & ~FriendlyPiecesBitboard;
}

uint64_t Attacks::GetRookMoves(int Square, const uint64_t AllOccupiedBitboard, const uint64_t FriendlyPiecesBitboard) const
{
	uint64_t BlockerBitboard = AllOccupiedBitboard & RookMasks[Square];
	uint64_t Key = (BlockerBitboard * RookMagics[Square]) >> RookShifts[Square];
	uint64_t AllMovesBitboard = RookMovesLookup[Square][Key];

	return AllMovesBitboard & ~FriendlyPiecesBitboard;
}

uint64_t Attacks::GetQueenMoves(int Square, const uint64_t AllOccupiedBitboard, const uint64_t FriendlyPiecesBitboard) const
{
	return GetBishopMoves(Square, AllOccupiedBitboard, FriendlyPiecesBitboard) |
		GetRookMoves(Square, AllOccupiedBitboard, FriendlyPiecesBitboard);
}

bool Attacks::IsAnyPieceAttacking(int Square, PieceColor Color, PieceType Type,
	uint64_t AttackingPiecesBitboard, const uint64_t AllOccupiedBitboard, const uint64_t FriendlyPiecesBitboard) const
{
	uint64_t(Attacks:: * GetMovesFunc)(int Square, const uint64_t AllOccupiedBitboard, const uint64_t FriendlyPiecesBitboard) const =
		Type == Bishop ? &Attacks::GetBishopMoves :
		Type == Rook ? &Attacks::GetRookMoves :
		Type == Queen ? &Attacks::GetQueenMoves :
		nullptr;

	while (AttackingPiecesBitboard)
	{
		int PieceSquare = pop_lsb(AttackingPiecesBitboard);

		uint64_t PieceMoves = (this->*GetMovesFunc)(PieceSquare, AllOccupiedBitboard, FriendlyPiecesBitboard);

		if (IsBitSet(PieceMoves, Square))
		{
			return true;
		}
	}

	return false;
}
