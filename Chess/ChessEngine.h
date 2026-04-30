#pragma once

#include "Includes.h"

#include "Piece.h"

struct Move
{
	int OldSquare;
	int NewSquare;

	Move()
		: OldSquare(-1), NewSquare(-1)
	{}

	Move(int InOldSquare, int InNewSquare)
		: OldSquare(InOldSquare), NewSquare(InNewSquare)
	{}

	Move(const char* AlgebraicOldSquare, const char* AlgebraicNewSquare) // no safety guaranteed
	{
		std::array<char, 2> OldSquareArray = { AlgebraicOldSquare[0], AlgebraicOldSquare[1] };
		std::array<char, 2> NewSquareArray = { AlgebraicNewSquare[0], AlgebraicNewSquare[1] };

		OldSquare = Piece::RankFileToSquare(Piece::AlgebraicToRankFile(OldSquareArray));
		NewSquare = Piece::RankFileToSquare(Piece::AlgebraicToRankFile(NewSquareArray));
	}

	const bool IsValid() const
	{
		return OldSquare != -1 && NewSquare != -1;
	}

	const bool IsZero() const
	{
		return OldSquare == NewSquare;
	}

	void Invalidate()
	{
		OldSquare = -1;
		NewSquare = -1;
	}
};

struct CastlingRights
{
	Move KingSide[2] = {
		Move{ e1, g1 }, // white
		Move{ e8, g8 }, // black
	};
	Move QueenSide[2] = {
		Move{ e1, c1 }, // white
		Move{ e8, c8 }, // black
	};
};

class ChessEngine
{
private:

	GLuint ImageTexture = 0;

	std::vector<std::unique_ptr<Piece>> Pieces;
	std::vector<std::unique_ptr<Piece>> CapturedPieces;

	ImVec2 SelectedPieceMouseOffset = { NAN, NAN };
	Piece* SelectedPiece = nullptr;

	PieceColor CurrentMove = White; // white starts by default
	CastlingRights CastlingRights;
	int EnpassantSquare = -1;
	int HalfMoveClock = 0;
	int FullMoveCounter = 1;
	Move LastMove = { -1, -1 };

#ifdef _DEBUG
	mutable std::vector<int> MarkedSquares; // cleared at the start of Draw()
#endif

public:

	ChessEngine(const char* FENString);

	// expects valid FEN string (no validity checking done)
	void LoadFENPosition(const char* FENString);

private:

	// this is ub since std::function doesn't follow the const correctness but it's fine since it's more convenient this way
	// for the predicate: return true if we should move on to the next square, return false if we should stop and return
	void IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const;

	std::vector<Move> GetAvailableMoves(Piece* TargetPiece) const;

	bool IsAllowedMove(
		Piece* MovingPiece,
		int NewSquare,
		decltype(Pieces)::const_iterator* OutCapturedPiece = nullptr,
		int* OutEnpassantSquare = nullptr,
		Move* OutCastledRookMove = nullptr
	) const;
	
	bool IsInCheck(PieceColor Color) const;
	bool IsAttacked(Piece* AttackedPiece) const;

	void RemoveCastlingRights(int MovedSquare, PieceColor Color);
	void FinishMove(Piece* MovingPiece);
	void TryMoveTo(Piece* MovingPiece, int NewSquare);

	void SnapSelectedPiece(const ImVec2& Pos);
	void UpdateSelectedPiece(const ImVec2& Pos);
	void SelectPiece(const ImVec2& Pos);

	void HandleInput();

	void CapturePiece(decltype(Pieces)::const_iterator CapturedPiece);

	Piece* GetPiece(int Rank, int File) const;
	Piece* GetPiece(int Square) const;

	Piece* GetFirstPiece(PieceType Type, PieceColor Color) const;

	int GetSquare(const ImVec2& Pos) const;

	std::pair<ImVec2, ImVec2> GetSquare(std::pair<int, int> RankFile) const;
	std::pair<ImVec2, ImVec2> GetSquare(int Square) const;

	decltype(Pieces)::const_iterator GetPieceIt(int Rank, int File) const;
	decltype(Pieces)::const_iterator GetPieceIt(int Square) const;

	ImVec2 GetSquareSize() const;

	void DrawPiece(const ImVec2& Pos, const ImVec2& PieceSize, PieceType Type, PieceColor Color, bool bCaptured) const;
	void DrawPiece(int Square, PieceType Type, PieceColor Color, bool bCaptured) const;
	void DrawPiece(const Piece& Piece, bool bCaptured) const;

#ifdef _DEBUG
	void DrawMarkedSquares() const;
#endif

	void DrawGrid() const;
	void DrawSelectedPiece() const;
	void DrawMoveInfo() const;
	void DrawAllowedMoves() const;
	void DrawPieces() const;
	void DrawCapturedPieces() const;

public:

	void Update();
	void Draw() const;
};