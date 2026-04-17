#pragma once

#include "Includes.h"

#include "Piece.h"

class ChessEngine
{
private:

	GLuint ImageTexture = 0;

	std::vector<std::unique_ptr<Piece>> Pieces;

	ImVec2 SelectedPieceMouseOffset;
	Piece* SelectedPiece;

	PieceColor CurrentMove = White; // white always starts
	int LastEnpassantSquare;

#ifdef _DEBUG
	std::vector<int> SquaresToMark;
#endif

public:

	ChessEngine();

private:

	// this is ub but it's fine
	// for the predicate: return true if we should move on to the next square, return false if we should stop and return
	void IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const;

	// if we are sure that we are going to move the piece if allowed, remove soon-to-be-captured pieces already
	bool IsAllowedMove(int NewSquare, bool bRemoveCapturedPieces);
	void FinishMove();

	void SnapSelectedPiece(const ImVec2& Pos);
	void UpdateSelectedPiece();
	void SelectPiece();
	void HandleInput();

	Piece* GetPiece(int Rank, int File) const;
	Piece* GetPiece(int Square) const;

	decltype(Pieces)::const_iterator GetPieceIt(int Rank, int File) const;
	decltype(Pieces)::const_iterator GetPieceIt(int Square) const;

	void DrawPiece(const ImVec2& Pos, PieceType Type, PieceColor Color) const;
	void DrawPiece(int Square, PieceType Type, PieceColor Color) const;
	void DrawPiece(const Piece& Piece) const;

	void DrawGrid() const;
	void DrawCurrentMove() const;
	void DrawPieces() const;

public:

	void Update();
	void Draw() const;
};