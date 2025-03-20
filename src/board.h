#pragma once
#include "hash.h"
#include "labels.h"
#include "moves.h"
#include "nnue.h"

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

    U64 getOccupied();
    U64 getWhitePieces();
    U64 getBlackPieces();
    U64 getUnsafeForWhite();
    U64 getUnsafeForBlack();

    int32_t evaluate_nnue();

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

    // nnue evaluation
    Accumulator accumulator;

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
    static void addMovesFromBitboard(MoveList& moves, U64 destinations, int position, BitBoards bb);
    static void addMovesFromBitboardPawnPromote(MoveList& moves, U64 movedBoard, int offset, BitBoards bb);
    static void addMovesFromBitboardAbsolute(MoveList& moves, U64 movedBoard, int offset, BitBoards bb, MoveTypes type);
};