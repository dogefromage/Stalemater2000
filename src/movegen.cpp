#include "bitmath.h"
#include "board.h"

/*
void Board::generateLegalMoves(MoveList& moveList) const {
    MoveList pseudoMoves;
    pseudoMoves.reserve(40);
    generatePseudoMoves(pseudoMoves);
    moveList.reserve(pseudoMoves.size()); // less or equal length than pseudomoves
    for (const GenMove& m : pseudoMoves) {
        Board testBoard = movePseudo(m);
        if (testBoard.isLegal()) {
            moveList.push_back(m);
        }
    }
}
*/

void Board::generatePseudoMoves(MoveList& moveList) {
    moveList.reserve(42);
    useDerivedState();

    genKnightMoves(moveList);  // horseys
    genKingMoves(moveList);    // king

    if (side == Side::White) {
        genMovesSlidingPieces(moveList, BitBoards::BW, false, true);  // bishops
        genMovesSlidingPieces(moveList, BitBoards::RW, true, false);  // rooks
        genMovesSlidingPieces(moveList, BitBoards::QW, true, true);   // queens
        genPawnMovesWhite(moveList);                                  // pawns
        genCastlesWhite(moveList);                                    // castles
    } else {
        genMovesSlidingPieces(moveList, BitBoards::BB, false, true);  // bishops
        genMovesSlidingPieces(moveList, BitBoards::RB, true, false);  // rooks
        genMovesSlidingPieces(moveList, BitBoards::QB, true, true);   // queens
        genPawnMovesBlack(moveList);                                  // pawns
        genCastlesBlack(moveList);                                    // castles
    };
}

void Board::genCastlesWhite(MoveList& moveList) const {
    if (castlingRights & (1 << (int)CastlingTypes::WhiteKing)) {
        bool lineUnderAttack = _unsafeForWhite & CASTLE_MASK_W_K_PATH;
        bool obstructed = _occupied & CASTLE_MASK_W_K_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.push_back(GenMove(4, 6, MovePromotions::None, BitBoards::KW, MoveTypes::CastleWhiteKing));
        }
    }
    if (castlingRights & (1 << (int)CastlingTypes::WhiteQueen)) {
        bool lineUnderAttack = _unsafeForWhite & CASTLE_MASK_W_Q_PATH;
        bool obstructed = _occupied & CASTLE_MASK_W_Q_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.push_back(GenMove(4, 2, MovePromotions::None, BitBoards::KW, MoveTypes::CastleWhiteQueen));
        }
    }
}

void Board::genCastlesBlack(MoveList& moveList) const {
    if (castlingRights & (1 << (int)CastlingTypes::BlackKing)) {
        bool lineUnderAttack = _unsafeForBlack & CASTLE_MASK_B_K_PATH;
        bool obstructed = _occupied & CASTLE_MASK_B_K_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.push_back(GenMove(60, 62, MovePromotions::None, BitBoards::KB, MoveTypes::CastleBlackKing));
        }
    }
    if (castlingRights & (1 << (int)CastlingTypes::BlackQueen)) {
        bool lineUnderAttack = _unsafeForBlack & CASTLE_MASK_B_Q_PATH;
        bool obstructed = _occupied & CASTLE_MASK_B_Q_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.push_back(GenMove(60, 58, MovePromotions::None, BitBoards::KB, MoveTypes::CastleBlackQueen));
        }
    }
}

void Board::genMovesSlidingPieces(MoveList& moveList, BitBoards bb, bool paral, bool diag) const {
    const U64& validToSquares =
        ~(side == Side::White ? _whitePieces : _blackPieces);
    U64 boardValue = boards[(int)bb];

    int i = 0;
    while (boardValue) {
        i = trailingZeros(boardValue);
        boardValue ^= 1ULL << i;  // unset this bit
        // -> piece at i
        U64 moves = 0;
        // find all moves
        if (paral) moves |= getHAndVMoves(i);
        if (diag) moves |= getDandAntiDMoves(i);
        // mask with board
        moves &= validToSquares;
        // add to list
        addMovesFromBitboard(moveList, moves, i, bb);
    }
}

void Board::genKingMoves(MoveList& moveList) const {
    bool isWhite = side == Side::White;
    BitBoards bb = isWhite ? BitBoards::KW : BitBoards::KB;
    U64 king = boards[(int)bb];

    const U64& validForWhite = ~(_whitePieces | _unsafeForWhite);
    const U64& validForBlack = ~(_blackPieces | _unsafeForBlack);
    const U64& validToSquares = isWhite ? validForWhite : validForBlack;

    while (king) {
        int i = trailingZeros(king);
        king ^= 1ULL << i;  // somehow necessary if multiple kings...
        // -> piece at i
        int offset = i - SPAN_KING_OFFSET;
        U64 moves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            moves &= ~FILE_H;
        else
            moves &= ~FILE_A;
        moves &= validToSquares;
        // add to list
        addMovesFromBitboard(moveList, moves, i, bb);
    }
}

void Board::genKnightMoves(MoveList& moveList) const {
    bool isWhite = side == Side::White;
    BitBoards bb = isWhite ? BitBoards::NW : BitBoards::NB;
    U64 horse = boards[(int)bb];
    const U64& validToSquares =
        ~(isWhite ? _whitePieces : _blackPieces);

    int i = 0;
    while (horse) {
        i = trailingZeros(horse);
        horse ^= 1ULL << i;  // unset this bit
        // -> piece at i
        int offset = i - SPAN_HORSE_OFFSET;
        U64 moves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset;  // weird stuff happens when shifting by neg number
        if (i % 8 < 4)
            moves &= ~FILE_GH;
        else
            moves &= ~FILE_AB;

        moves &= validToSquares;
        // add to list
        addMovesFromBitboard(moveList, moves, i, bb);
    }
}

void Board::genPawnMovesWhite(MoveList& moveList) const {
    const U64& pawns = boards[(int)BitBoards::PW];
    U64 pawnMoves;
    U64 empty = ~_occupied;
    // queenwards capture
    pawnMoves = (pawns << 7) & ~FILE_H & ~RANK_8 & (_blackPieces | enpassantTarget);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & ~enpassantTarget, 7, BitBoards::PW, MoveTypes::Normal);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & enpassantTarget, 7, BitBoards::PW, MoveTypes::EnpasQueen);
    // kingwards capture
    pawnMoves = (pawns << 9) & ~FILE_A & ~RANK_8 & (_blackPieces | enpassantTarget);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & ~enpassantTarget, 9, BitBoards::PW, MoveTypes::Normal);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & enpassantTarget, 9, BitBoards::PW, MoveTypes::EnpasKing);
    // Forward one
    pawnMoves = (pawns << 8) & ~RANK_8 & empty;
    addMovesFromBitboardAbsolute(moveList, pawnMoves, 8, BitBoards::PW, MoveTypes::Normal);
    // Forward two
    pawnMoves = (((pawns << 8) & empty) << 8) & WHITE_SIDE & empty;
    addMovesFromBitboardAbsolute(moveList, pawnMoves, 16, BitBoards::PW, MoveTypes::PawnDouble);  // add move type

    // Promote diag left
    pawnMoves = (pawns << 7) & ~FILE_H & RANK_8 & _blackPieces;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, 7, BitBoards::PW);
    // Promote diag right
    pawnMoves = (pawns << 9) & ~FILE_A & RANK_8 & _blackPieces;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, 9, BitBoards::PW);
    // Promote forward
    pawnMoves = (pawns << 8) & RANK_8 & empty;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, 8, BitBoards::PW);
}

void Board::genPawnMovesBlack(MoveList& moveList) const {
    const U64& pawns = boards[(int)BitBoards::PB];
    U64 pawnMoves;
    U64 empty = ~_occupied;
    // kingwards
    pawnMoves = (pawns >> 7) & ~FILE_A & ~RANK_1 & (_whitePieces | enpassantTarget);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & ~enpassantTarget, -7, BitBoards::PB, MoveTypes::Normal);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & enpassantTarget, -7, BitBoards::PB, MoveTypes::EnpasKing);
    // queenwards
    pawnMoves = (pawns >> 9) & ~FILE_H & ~RANK_1 & (_whitePieces | enpassantTarget);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & ~enpassantTarget, -9, BitBoards::PB, MoveTypes::Normal);
    addMovesFromBitboardAbsolute(moveList, pawnMoves & enpassantTarget, -9, BitBoards::PB, MoveTypes::EnpasQueen);
    // Forward one
    pawnMoves = (pawns >> 8) & ~RANK_1 & empty;
    addMovesFromBitboardAbsolute(moveList, pawnMoves, -8, BitBoards::PB, MoveTypes::Normal);
    // Forward two
    pawnMoves = (((pawns >> 8) & empty) >> 8) & BLACK_SIDE & empty;
    addMovesFromBitboardAbsolute(moveList, pawnMoves, -16, BitBoards::PB, MoveTypes::PawnDouble);  // add move type

    // Promote diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & RANK_1 & _whitePieces;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, -7, BitBoards::PB);
    // Promote diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & RANK_1 & _whitePieces;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, -9, BitBoards::PB);
    // Promote forward
    pawnMoves = (pawns >> 8) & RANK_1 & empty;
    addMovesFromBitboardPawnPromote(moveList, pawnMoves, -8, BitBoards::PB);
}

// maybe use table for this part
U64 Board::getHAndVMoves(int index) const {
    U64 s = 1ULL << index;
    int i = index % 8, j = index / 8;

    // find all moves by magic
    U64 horizontal = (_occupied - 2 * s) ^ reverse(reverse(_occupied) - 2 * reverse(s));
    U64 vertical = ((_occupied & FILE_MASKS[i]) - 2 * s) ^ reverse(reverse(_occupied & FILE_MASKS[i]) - 2 * reverse(s));
    return (horizontal & RANK_MASKS[j]) | (vertical & FILE_MASKS[i]);
}

U64 Board::getDandAntiDMoves(int index) const {
    U64 s = 1ULL << index;
    int d = (index / 8) + (index % 8);
    int ad = (index / 8) + 7 - (index % 8);

    // find all moves by magic
    U64 diag = ((_occupied & DIAG_MASK[d]) - 2 * s) ^ reverse(reverse(_occupied & DIAG_MASK[d]) - 2 * reverse(s));
    U64 antiDiag = ((_occupied & ANTIDIAG_MASK[ad]) - 2 * s) ^ reverse(reverse(_occupied & ANTIDIAG_MASK[ad]) - 2 * reverse(s));
    return (diag & DIAG_MASK[d]) | (antiDiag & ANTIDIAG_MASK[ad]);
}

void Board::addMovesFromBitboard(MoveList& moves, U64 destinations, int position, BitBoards bb) {
    int i = 0;
    while (destinations) {
        i = trailingZeros(destinations);
        destinations ^= 1ULL << i;  // unset this bit
        moves.push_back(GenMove(position, i, MovePromotions::None, bb, MoveTypes::Normal));
    }
}

void Board::addMovesFromBitboardPawnPromote(MoveList& moves, U64 movedBoard, int offset, BitBoards bb) {
    int i = 0;
    while (movedBoard) {
        i = trailingZeros(movedBoard);
        movedBoard ^= 1ULL << i;  // unset this bit
        moves.push_back(GenMove(i - offset, i, MovePromotions::Q, bb, MoveTypes::Promote));
        moves.push_back(GenMove(i - offset, i, MovePromotions::R, bb, MoveTypes::Promote));
        moves.push_back(GenMove(i - offset, i, MovePromotions::N, bb, MoveTypes::Promote));
        moves.push_back(GenMove(i - offset, i, MovePromotions::B, bb, MoveTypes::Promote));
    }
}

void Board::addMovesFromBitboardAbsolute(MoveList& moves, U64 movedBoard, int offset, BitBoards bb, MoveTypes type) {
    int i = 0;
    while (movedBoard) {
        i = trailingZeros(movedBoard);
        movedBoard ^= 1ULL << i;  // unset this bit

        moves.push_back(GenMove(i - offset, i, MovePromotions::None, bb, type));
    }
}
