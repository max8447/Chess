#pragma once

#include "Includes.h"

#include "Piece.h"
#include "Move.h"
#include "Transposition.h"
#include "Bitboard.h"

#define SEARCHBESTMOVE_MIN -999999
#define SEARCHBESTMOVE_MAX 999999

struct CastlingRights
{
	SpecialMove KingSide[PieceColor_Count] = {
		SpecialMove{ { e1, g1 }, Castle, { h1, f1 } },	// white
		SpecialMove{ { e8, g8 }, Castle, { h8, f8 } },	// black
	};
	SpecialMove QueenSide[PieceColor_Count] = {
		SpecialMove{ { e1, c1 }, Castle, { a1, d1 } },	// white
		SpecialMove{ { e8, c8 }, Castle, { a8, d8 } },	// black
	};

	void Invalidate()
	{
		KingSide[0].Invalidate();
		KingSide[1].Invalidate();
		QueenSide[0].Invalidate();
		QueenSide[1].Invalidate();
	}
};

class ChessEngine
{
private:

	GLuint PiecesImageTexture = 0;

	const char* FENString;

	std::vector<std::unique_ptr<Piece>> Pieces;

	ImVec2 SelectedPieceMouseOffset = { NAN, NAN };
	Piece* SelectedPiece = nullptr;

	std::vector<SpecialMove> AllAvailableMoves;
	std::vector<SpecialMove> AvailableMoves;
	int NumPossibleMoves = -1;
	bool bGameEnded = false;

	// game state

	PieceColor CurrentMove = White; // white starts by default
	mutable CastlingRights AvailableCastlingRights; // mutable for now
	mutable int EnpassantSquare = -1;				// mutable for now
	int PawnPromotionSquare = -1;
	int HalfMoveClock = 0;
	int FullMoveCounter = 1;
	SpecialMove LastMove = SpecialMove{};

	mutable uint64_t CurrentHash;
	mutable TranspositionTable TranspositionTable;

	mutable Bitboard Bitboard;
	Attacks Attacks;

	// TODO: make these selectable
	
	PieceColor PlayerColor = White;
	PieceColor BotColor = Black;

	bool bEnableBot = true;

	mutable int MoveGenerationTestPossibleMoves = -1;

#ifdef _DEBUG
	mutable std::vector<int> MarkedSquares; // cleared at the start of Draw() and drawn at the end of Draw()
#endif

public:

	ChessEngine(const char* FENString);
	~ChessEngine();

	// expects valid FEN string (no validity checking done)
	void LoadFENPosition(const char* InFENString);
	char* GenerateFENPosition();

	void InitBitboard();

private:

	// this is ub since std::function doesn't follow the const correctness but it's fine since it's more convenient this way
	// for the predicate: return true if we should move on to the next square, return false if we should stop and return
	void IterateSquares(std::function<bool(const ImVec2& Min, const ImVec2& Max, int Rank, int File)> Predicate) const;

	// move generation functions

	void GetAllScoredMoves(PieceColor Color, std::vector<ScoredMove>* OutScoredMoves, bool bAllowPseudolegalMoves = false) const;

	void GetAllAvailableMoves(PieceColor Color, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves = false) const;
	void GetAllAvailableMoves(PieceColor Color, SpecialMove* OutAvailableMoves, int& OutAvailableMovesCount, bool bAllowPseudolegalMoves = false) const;

	void GetScoredMoves(Piece* TargetPiece, std::vector<ScoredMove>* OutScoredMoves, bool bAllowPseudolegalMoves = false) const;

	void GetAvailableMoves(Piece* TargetPiece, std::vector<SpecialMove>* OutAvailableMoves, bool bAllowPseudolegalMoves = false) const;
	void GetAvailableMoves(Piece* TargetPiece, SpecialMove* OutAvailableMoves, int& OutAvailableMovesCount, bool bAllowPseudolegalMoves = false) const;

	void CalculatePossibleMoves();

	bool IsAllowedMove(
		Piece* MovingPiece,
		int NewSquare,
		bool bAllowPseudolegal,
		SpecialMove* OutSpecialMove
	) const;

	void MakeMove(const SpecialMove& Move, CastlingRights* OutCastlingRights = nullptr, int* OutEnpassantSquare = nullptr, uint64_t* OutHash = nullptr) const;
	void UnMakeMove(const SpecialMove& Move) const;

	void MakeBitboardsMove(const SpecialMove& Move) const;
	void UnMakeBitboardsMove(const SpecialMove& Move) const;

	bool IsInCheck(PieceColor Color) const;
	bool IsAttacked(int AttackedSquare, PieceColor Color) const;
	bool IsAttacked(Piece* AttackedPiece) const;

	// universal move-making functions

	void RemoveCastlingRights(Piece* MovedPiece, int NewSquare, CastlingRights& OutCastlingRights) const;

	void FinishMove(Piece* MovingPiece, const SpecialMove& Move);
	void TryMoveTo(Piece* MovingPiece, int NewSquare);
	void CapturePiece(Piece* CapturedPiece);

	// bot functions

	void HandleBotPawnPromotion();
	void GenerateMove();
	int GetMaterialValue(PieceColor Color) const;
	int GetMobility(PieceColor Color) const;
	int EvaluatePosition() const;
	int GuessMoveScore(const SpecialMove& Move) const;
	int SearchBestMove(SpecialMove& OutBestMove, PieceColor Color, int Depth, int Alpha = SEARCHBESTMOVE_MIN, int Beta = SEARCHBESTMOVE_MAX) const;
	int MoveGenerationTest(PieceColor Color, int Depth, bool bIsRoot);

	// transposition functions

	uint64_t GenerateHash(PieceColor Move, const CastlingRights& CastlingRights, int EnpassantSquare) const;

	// player functions

	void SnapSelectedPiece(const ImVec2& Pos);
	void UpdateSelectedPiece(const ImVec2& Pos);
	void SelectPiece(const ImVec2& Pos);

	void HandleInput();

	Piece* GetPiece(int Rank, int File) const;
	Piece* GetPiece(int Square) const;

	int GetSquare(const ImVec2& Pos) const;

	std::pair<ImVec2, ImVec2> GetSquare(std::pair<int, int> RankFile) const;
	std::pair<ImVec2, ImVec2> GetSquare(int Square) const;

	ImVec2 GetSquareSize() const;

	void DrawPiece(const ImVec2& Pos, const ImVec2& PieceSize, PieceType Type, PieceColor Color) const;
	void DrawPiece(int Square, PieceType Type, PieceColor Color) const;
	void DrawPiece(const Piece& Piece) const;

#ifdef _DEBUG
	void DrawMarkedSquares() const;
#endif

	void DrawGrid() const;
	void DrawSelectedPiece() const;
	void DrawMoveInfo() const;
	void DrawAvailableMoves() const;
	void DrawPieces() const;
	void DrawPromotionPopup() const;
	void DrawEndScreen() const;

public:

	void Update();
	void Draw() const;
};