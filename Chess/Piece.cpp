#include "Piece.h"

int Piece::GetDirection() const
{
	return Color == White ? 1 : -1;
}

std::pair<int, int> Piece::GetRankFile(int Square) const
{
	if (Square == -1)
	{
		Square = this->Square;
	}

	const auto [Rank, File] = SquareToRankFile(Square);

	if (Color == Black)
	{
		return { 7 - Rank, 7 - File };
	}
	else
	{
		return { Rank, File };
	}
}

int Piece::GetSquare(int Rank, int File) const
{
	if (Rank == -1 || File == -1)
	{
		Rank = SquareToRankFile(Square).first;
		File = SquareToRankFile(Square).second;
	}

	if (Color == Black)
	{
		Rank = 7 - Rank;
		File = 7 - File;
	}

	return RankFileToSquare(Rank, File);
}

bool Piece::IsAllowedMove(int NewSquare) const
{
	std::array<int, 9> AllowedMoves = GetAvailableMoves(Type);

	const auto [OldRank, OldFile] = GetRankFile();
	const auto [NewRank, NewFile] = GetRankFile(NewSquare);
	
	int DeltaRank = NewRank - OldRank;
	int DeltaFile = NewFile - OldFile;

	if (DeltaRank == 0 && DeltaFile == 0)
	{
		return false; // no move was made
	}

	int IdxToCheck = 4; // always 0 (center)

	if (DeltaRank == 0)
	{
		IdxToCheck = DeltaFile < 0 ? 3 : 5;
	}
	else if (DeltaFile == 0)
	{
		IdxToCheck = DeltaRank < 0 ? 7 : 1;
	}
	else
	{
		if (DeltaRank > 0 && DeltaFile < 0)
		{
			IdxToCheck = 0;
		}
		else if (DeltaRank > 0 && DeltaFile > 0)
		{
			IdxToCheck = 2;
		}
		else if (DeltaRank < 0 && DeltaFile < 0)
		{
			IdxToCheck = 6;
		}
		else if (DeltaRank < 0 && DeltaFile > 0)
		{
			IdxToCheck = 8;
		}
	}

	IM_ASSERT(IdxToCheck != 4);

	int AllowedSquares = AllowedMoves[IdxToCheck];

	DeltaRank = abs(DeltaRank);
	DeltaFile = abs(DeltaFile);

	bool bCheckDiagonal = IdxToCheck % 2 == 0;

	if (bCheckDiagonal)
	{
		if (Type == Knight)
		{
			return max(DeltaRank, DeltaFile) == 2 &&
				min(DeltaRank, DeltaFile) == 1;
		}

		if (DeltaRank != DeltaFile)
		{
			return false; // diagonal moves will always be at a 45 degree angle (except for knight)
		}

		return DeltaRank <= AllowedSquares;
	}
	else
	{
		if (DeltaRank == 0)
		{
			return DeltaFile <= AllowedSquares;
		}
		else // DeltaFile == 0
		{
			return DeltaRank <= AllowedSquares;
		}
	}

	return false;
}

constexpr std::array<int, 9> Piece::GetAvailableMoves(PieceType Type)
{
	std::array<int, 9> AllowedMoves = {
		0, 0, 0,
		0, 0, 0,
		0, 0, 0
	};

	switch (Type)
	{
	case King:
		AllowedMoves = {
			1, 1, 1,
			1, 0, 1,
			1, 1, 1
		};

		break;
	case Queen:
		AllowedMoves = {
			8, 8, 8,
			8, 0, 8,
			8, 8, 8
		};

		break;
	case Bishop:
		AllowedMoves = {
			8, 0, 8,
			0, 0, 0,
			8, 0, 8
		};

		break;
	case Knight:
		// Knight has its own special implementation in IsAllowedMove

		break;
	case Rook:
		AllowedMoves = {
			0, 8, 0,
			8, 0, 8,
			0, 8, 0
		};

		break;
	case Pawn:
		AllowedMoves = {
			0, 1, 0,
			0, 0, 0,
			0, 0, 0
		};

		break;
	default:
		break;
	}

	return AllowedMoves;
}