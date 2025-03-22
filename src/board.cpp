#include "board.h"

#include <cassert>

#include "bitmath.h"

void Board::forbidCastling(CastlingTypes castling) {
    assert(0 <= (int)castling && (int)castling < 4);
    int c = (int)castling;
    int mask = 1 << c;
    if (castlingRights & mask) {
        castlingRights &= ~mask;
        hash ^= ZobristValues[ZOBRIST_CASTLING + c];
    }
}

Side Board::getSideToMove() const {
    return side;
}

U64 Board::getBoard(BitBoards bb) const {
    return boards[(int)bb];
}

bool Board::getCastlingRight(CastlingTypes ct) const {
    return (castlingRights & (1 << (int)ct)) != 0;
}

U64 Board::getEnpassantTarget() const {
    return enpassantTarget;
}

U64 Board::getHash() const {
    return hash;
}

bool Board::hasCheck(CheckFlags checkingSide) const {
    return (_checks & (int)checkingSide) != 0;
}

bool Board::movePieceOrCapture(BitBoards bb, int from, int to) {
    useDerivedState();

    U64 toMask = 1ULL << to;
    bool isCapture = false;
    // test capture
    if (toMask & _occupied) {
        // only other pieces must be checked for capture
        int offset = 0;
        if ((int)bb < 6) {
            offset = 6;
        }
        for (int i = offset; i < offset + 6; i++) {
            if (toMask & boards[i]) {
                // turn captured bit off
                removePiece((BitBoards)i, to);
                isCapture = true;
                break;  // only one capture possible :)
            }
        }
        // this would fail if occupied wrong or friendly fire capture
        assert(isCapture);
    }
    // more piece
    removePiece(bb, from);
    placePiece(bb, to);

    return isCapture;
}

void Board::placePiece(BitBoards bb, int square) {
    // piece should not exists
    assert(~(boards[(int)bb] & (1ULL << square)));
    boards[(int)bb] |= 1ULL << square;
    hash ^= ZobristValues[64 * (int)bb + square];
    accumulator.add((int)bb, square);
}

void Board::removePiece(BitBoards bb, int square) {
    // piece should exist
    assert(boards[(int)bb] & (1ULL << square));
    boards[(int)bb] &= ~(1ULL << square);
    hash ^= ZobristValues[64 * (int)bb + square];
    accumulator.remove((int)bb, square);
}

void Board::switchSide() {
    side = (Side)((int)side ^ 1);
    hash ^= ZobristValues[ZOBRIST_BLACK_MOVE];
}

void Board::setEnpassantTarget(U64 newTarget) {
    if (enpassantTarget) {
        // remove last hash
        hash ^= ZobristValues[ZOBRIST_ENPASSANT + trailingZeros(enpassantTarget)];
    }
    enpassantTarget = newTarget;
    if (enpassantTarget) {
        // add new hash
        hash ^= ZobristValues[ZOBRIST_ENPASSANT + trailingZeros(enpassantTarget)];
    }
}

void Board::sanityCheck() {
    assert(isLegal());

    // check square occupations
    for (int i = 0; i < 64; i++) {
        U64 mask = 1ULL << i;
        bool occupied = false;
        for (int j = 0; j < 12; j++) {
            if (boards[j] & mask) {
                // only one piece can occupy a square
                assert(!occupied);
                occupied = true;
            }
        }
    }

    // no pawns should every be on outer ranks

    assert((boards[(int)BitBoards::PW] & RANK_1 & RANK_8) == 0);
    assert((boards[(int)BitBoards::PB] & RANK_1 & RANK_8) == 0);

    assert(countBits(boards[(int)BitBoards::KW]) == 1);
    assert(countBits(boards[(int)BitBoards::KB]) == 1);
}

bool Board::isLegal() {
    useDerivedState();
    bool whiteCheck = _checks & (char)CheckFlags::WhiteInCheck;
    bool blackCheck = _checks & (char)CheckFlags::BlackInCheck;
    if (side == Side::Black && whiteCheck) return false;  // illegal
    if (side == Side::White && blackCheck) return false;
    return true;
}

void Board::useDerivedState() {
    if (hash == _lastDerivedHash) {
        // board has not changed
        return;
    }

    _whitePieces = boards[0] | boards[1] | boards[2] | boards[3] | boards[4] | boards[5];
    _blackPieces = boards[6] | boards[7] | boards[8] | boards[9] | boards[10] | boards[11];
    _occupied = _whitePieces | _blackPieces;

    _unsafeForWhite = findUnsafeForWhite();
    _unsafeForBlack = findUnsafeForBlack();

    _checks = 0;
    if (boards[(int)BitBoards::KW] & _unsafeForWhite) {
        _checks |= (int)CheckFlags::WhiteInCheck;
    }
    if (boards[(int)BitBoards::KB] & _unsafeForBlack) {
        _checks |= (int)CheckFlags::BlackInCheck;
    }

    _lastDerivedHash = hash;
}

U64 Board::findUnsafeForWhite() const {
    U64 unsafe = 0;
    // pawns
    unsafe |= (boards[(int)BitBoards::PB] >> 7) & ~FILE_A;
    unsafe |= (boards[(int)BitBoards::PB] >> 9) & ~FILE_H;
    // BISHOPS
    int i = 0;
    U64 bishops = boards[(int)BitBoards::BB];
    while (bishops) {
        i = trailingZeros(bishops);
        bishops ^= 1ULL << i;  // unset this bit
        U64 bishopMoves = getDandAntiDMoves(i);
        unsafe |= bishopMoves;
    }
    // ROOKS
    i = 0;
    U64 rooks = boards[(int)BitBoards::RB];
    while (rooks) {
        i = trailingZeros(rooks);
        rooks ^= 1ULL << i;  // unset this bit
        unsafe |= getHAndVMoves(i);
    }
    // QUEENS
    i = 0;
    U64 queens = boards[(int)BitBoards::QB];
    while (queens) {
        i = trailingZeros(queens);
        queens ^= 1ULL << i;  // unset this bit
        unsafe |= getHAndVMoves(i) | getDandAntiDMoves(i);
    }
    // HORSES
    i = 0;
    U64 horses = boards[(int)BitBoards::NB];
    while (horses) {
        i = trailingZeros(horses);
        horses ^= 1ULL << i;  // unset this bit
        int offset = i - SPAN_HORSE_OFFSET;
        U64 horseMoves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            horseMoves &= ~FILE_GH;
        else
            horseMoves &= ~FILE_AB;
        unsafe |= horseMoves;
    }
    // KING
    i = 0;
    U64 king = boards[(int)BitBoards::KB];
    if (king) {
        i = trailingZeros(king);
        king ^= 1ULL << i;  // unset this bit
        int offset = i - SPAN_KING_OFFSET;
        U64 kingMoves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            kingMoves &= ~FILE_H;
        else
            kingMoves &= ~FILE_A;
        unsafe |= kingMoves;
    }
    return unsafe;
}

U64 Board::findUnsafeForBlack() const {
    U64 unsafe = 0;
    // pawns
    unsafe |= (boards[(int)BitBoards::PW] << 7) & ~FILE_H;
    unsafe |= (boards[(int)BitBoards::PW] << 9) & ~FILE_A;

    // BISHOPS
    int i = 0;
    U64 bishops = boards[(int)BitBoards::BW];
    while (bishops) {
        i = trailingZeros(bishops);
        bishops ^= 1ULL << i;  // unset this bit
        U64 bishopMoves = getDandAntiDMoves(i);
        unsafe |= bishopMoves;
    }
    // ROOKS
    i = 0;
    U64 rooks = boards[(int)BitBoards::RW];
    while (rooks) {
        i = trailingZeros(rooks);
        rooks ^= 1ULL << i;  // unset this bit
        unsafe |= getHAndVMoves(i);
    }
    // QUEENS
    i = 0;
    U64 queens = boards[(int)BitBoards::QW];
    while (queens) {
        i = trailingZeros(queens);
        queens ^= 1ULL << i;  // unset this bit
        unsafe |= getHAndVMoves(i) | getDandAntiDMoves(i);
    }
    // HORSES
    i = 0;
    U64 horses = boards[(int)BitBoards::NW];
    while (horses) {
        i = trailingZeros(horses);
        horses ^= 1ULL << i;  // unset this bit
        int offset = i - SPAN_HORSE_OFFSET;
        U64 horseMoves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            horseMoves &= ~FILE_GH;
        else
            horseMoves &= ~FILE_AB;
        unsafe |= horseMoves;
    }
    // KING
    i = 0;
    U64 king = boards[(int)BitBoards::KW];
    if (king) {
        i = trailingZeros(king);
        king ^= 1ULL << i;  // unset this bit
        int offset = i - SPAN_KING_OFFSET;
        U64 kingMoves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            kingMoves &= ~FILE_H;
        else
            kingMoves &= ~FILE_A;
        unsafe |= kingMoves;
    }
    return unsafe;
}

U64 Board::getOccupied() {
    useDerivedState();
    return _occupied;
}

U64 Board::getWhitePieces() {
    useDerivedState();
    return _whitePieces;
}

U64 Board::getBlackPieces() {
    useDerivedState();
    return _blackPieces;
}

U64 Board::getUnsafeForWhite() {
    useDerivedState();
    return _unsafeForWhite;
}

U64 Board::getUnsafeForBlack() {
    useDerivedState();
    return _unsafeForBlack;
}

int32_t Board::evaluate_nnue() {
    return accumulator.forward(side, getOccupied());
}
