#pragma once

#include "Includes.h"

#include "Piece.h"

struct Move
{
	int OldSquare;
	int NewSquare;
};

class ChessEngine
{
private:

	GLuint ImageTexture = 0;

	std::vector<std::unique_ptr<Piece>> Pieces;

	ImVec2 SelectedPieceMouseOffset = { NAN, NAN };
	Piece* SelectedPiece = nullptr;
	decltype(Pieces)::const_iterator CapturedPiece;

	PieceColor CurrentMove = White; // white starts by default
	int EnpassantSquare = -1;
	int HalfMoveClock = 0;
	int FullMoveCounter = 1;
	Move LastMove = { -1, -1 };

#ifdef _DEBUG
	mutable std::vector<int> MarkedSquares; // cleared at the start of Draw()
#endif

public:

	ChessEngine();

	// expects valid FEN string (no validity checking done)
	void LoadFENPosition(const char* FENString);

private:

	// this is ub since std::function doesn't follow the const correctness but it's fine since it's more convenient this way
	// for the predicate: return true if we should move on to the next square, return false if we should stop and return
	void IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const;

	bool IsAllowedMove(int NewSquare, decltype(Pieces)::const_iterator* OutCapturedPiece, int* OutEnpassantSquare) const;
	void FinishMove();

	void SnapSelectedPiece(const ImVec2& Pos);
	void UpdateSelectedPiece(const ImVec2& Pos);
	void SelectPiece(const ImVec2& Pos);
	void HandleInput();

	void CapturePiece();

	Piece* GetPiece(int Rank, int File) const;
	Piece* GetPiece(int Square) const;

	int GetSquare(const ImVec2& Pos) const;

	std::pair<ImVec2, ImVec2> GetSquare(std::pair<int, int> RankFile) const;
	std::pair<ImVec2, ImVec2> GetSquare(int Square) const;

	decltype(Pieces)::const_iterator GetPieceIt(int Rank, int File) const;
	decltype(Pieces)::const_iterator GetPieceIt(int Square) const;

	void DrawPiece(const ImVec2& Pos, PieceType Type, PieceColor Color) const;
	void DrawPiece(int Square, PieceType Type, PieceColor Color) const;
	void DrawPiece(const Piece& Piece) const;

#ifdef _DEBUG
	void DrawMarkedSquares() const;
#endif

	void DrawGrid() const;
	void DrawSelectedPiece() const;
	void DrawMoveInfo() const;
	void DrawAllowedMoves() const;
	void DrawPieces() const;

public:

	void Update();
	void Draw() const;
};