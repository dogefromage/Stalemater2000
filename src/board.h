#pragma once
#include "hash.h"
#include "labels.h"
#include "moves.h"

enum class BoardEditType {
    Add,
    Remove,
};

struct BoardEdit {
    BoardEditType type;
    int bb, square;

    BoardEdit() {}
    BoardEdit(BoardEditType type, int bb, int square) : type(type), bb(bb), square(square) {}
};

struct BoardEditRecorder {
    int numEdits = 0;
    BoardEdit edits[MAX_BOARD_EDITS_PER_MOVE];
    
    void record(BoardEdit edit);
    void clear();
};

class Board {
public:
    Side getSideToMove() const;
    U64 getBoard(BitBoards bb) const;
    bool getCastlingRight(CastlingTypes ct) const;
    U64 getEnpassantTarget() const;
    U64 getHash() const;
    bool hasCheck(CheckFlags checkingSide) const;

    bool movePieceOrCapture(BitBoards bb, int from, int to);
    void forbidCastling(CastlingTypes castling);
    void placePiece(BitBoards bb, int square);
    void removePiece(BitBoards bb, int square);
    void switchSide();
    void setEnpassantTarget(U64 newTarget);

    void sanityCheck();
    bool isLegal();

    void generatePseudoMoves(MoveList& moveList);
    void orderAndFilterMoveList(MoveList& moveList, const LanMove& pv, bool capturesOnly) const;

    U64 getOccupied();
    U64 getWhitePieces();
    U64 getBlackPieces();
    U64 getUnsafeForWhite();
    U64 getUnsafeForBlack();

    BoardEditRecorder* editRecorder;

private:
    U64 boards[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    U64 enpassantTarget = 0;
    char castlingRights = 0xf;
    Side side = Side::White;
    U64 hash = 0;

    // derived state (always underscored)
    U64 _lastDerivedHash = 1;
    U64 _occupied = 0, _whitePieces = 0, _blackPieces = 0, _unsafeForWhite = 0, _unsafeForBlack = 0;
    char _checks = 0;

    void useDerivedState();
    U64 findUnsafeForWhite() const;
    U64 findUnsafeForBlack() const;

    void genCastlesWhite(MoveList& moves) const;
    void genCastlesBlack(MoveList& moves) const;
    void genMovesSlidingPieces(MoveList& moveList, BitBoards bb, bool paral, bool diag) const;
    void genKingMoves(MoveList& moves) const;
    void genKnightMoves(MoveList& moves) const;
    void genPawnMovesWhite(MoveList& moves) const;
    void genPawnMovesBlack(MoveList& moves) const;
    U64 getHAndVMoves(int index) const;
    U64 getDandAntiDMoves(int index) const;
    void addMovesFromBitboardSingle(MoveList& moves, U64 destinations, int position, BitBoards bb) const;
    void addMovesFromBitboardParallelPromote(MoveList& moves, U64 destinations, int offset, BitBoards bb) const;
    void addMovesFromBitboardParallel(MoveList& moves, U64 destinations, int offset, BitBoards bb, MoveTypes type) const;
};
