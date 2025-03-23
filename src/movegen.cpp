#include "bitmath.h"
#include "board.h"

void Board::generatePseudoMoves(MoveList& moveList) {
    useDerivedState();

    genKnightMoves(moveList);
    genKingMoves(moveList);

    if (side == Side::White) {
        genMovesSlidingPieces(moveList, BitBoards::BW, false, true);
        genMovesSlidingPieces(moveList, BitBoards::RW, true, false);
        genMovesSlidingPieces(moveList, BitBoards::QW, true, true);
        genPawnMovesWhite(moveList);
        genCastlesWhite(moveList);
    } else {
        genMovesSlidingPieces(moveList, BitBoards::BB, false, true);
        genMovesSlidingPieces(moveList, BitBoards::RB, true, false);
        genMovesSlidingPieces(moveList, BitBoards::QB, true, true);
        genPawnMovesBlack(moveList);
        genCastlesBlack(moveList);
    };
}

void Board::orderAndFilterMoveList(MoveList& moveList, const LanMove& pv, bool capturesOnly) const {

    /**
     * TODO: this could be sped up by directly taking the optimal move from an iterator automatically sorts and returns a reference. This would avoid copying
     */

    int numCaptures = 0;

    // score all moves
    for (GenMove& m : moveList) {
        bool isCapture = m.capture == CaptureType::Capture;
        if (isCapture) {
            numCaptures++;
        }

        if (m.matchesLanMove(pv)) {
            // PV
            m.score = 0;
        } else if (m.type == MoveTypes::Promote) {
            // Promotion
            m.score = 1;
        } else if (isCapture) {
            // Capture
            m.score = 2;
        } else {
            // Other
            m.score = 3;
        }

        if (capturesOnly && !isCapture) {
            // overwrite other score since not capture here
            m.score = 10;
        }
    }

    int sortSpace = capturesOnly ? numCaptures : moveList.size - 1;

    // sort
    for (int i = 0; i < sortSpace; i++) {

        int minScore = moveList.list[i].score;
        int minIndex = i;

        for (int j = i + 1; j < moveList.size; j++) {
            int currScore = moveList.list[j].score;
            if (currScore < minScore) {
                minScore = currScore;
                minIndex = j;
            }
        }

        // swap
        if (minIndex != i) {
            GenMove temp = moveList.list[i];
            moveList.list[i] = moveList.list[minIndex];
            moveList.list[minIndex] = temp;
        }
    }    

    // prune
    if (capturesOnly) {
        moveList.size = numCaptures;
    }
}

void Board::genCastlesWhite(MoveList& moveList) const {
    if (castlingRights & (1 << (int)CastlingTypes::WhiteKing)) {
        bool lineUnderAttack = _unsafeForWhite & CASTLE_MASK_W_K_PATH;
        bool obstructed = _occupied & CASTLE_MASK_W_K_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.add(GenMove(4, 6, MovePromotions::None, BitBoards::KW, MoveTypes::CastleWhiteKing, CaptureType::NonCapture));
        }
    }
    if (castlingRights & (1 << (int)CastlingTypes::WhiteQueen)) {
        bool lineUnderAttack = _unsafeForWhite & CASTLE_MASK_W_Q_PATH;
        bool obstructed = _occupied & CASTLE_MASK_W_Q_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.add(GenMove(4, 2, MovePromotions::None, BitBoards::KW, MoveTypes::CastleWhiteQueen, CaptureType::NonCapture));
        }
    }
}

void Board::genCastlesBlack(MoveList& moveList) const {
    if (castlingRights & (1 << (int)CastlingTypes::BlackKing)) {
        bool lineUnderAttack = _unsafeForBlack & CASTLE_MASK_B_K_PATH;
        bool obstructed = _occupied & CASTLE_MASK_B_K_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.add(GenMove(60, 62, MovePromotions::None, BitBoards::KB, MoveTypes::CastleBlackKing, CaptureType::NonCapture));
        }
    }
    if (castlingRights & (1 << (int)CastlingTypes::BlackQueen)) {
        bool lineUnderAttack = _unsafeForBlack & CASTLE_MASK_B_Q_PATH;
        bool obstructed = _occupied & CASTLE_MASK_B_Q_GAP;
        if (!lineUnderAttack && !obstructed) {
            moveList.add(GenMove(60, 58, MovePromotions::None, BitBoards::KB, MoveTypes::CastleBlackQueen, CaptureType::NonCapture));
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
        addMovesFromBitboardSingle(moveList, moves, i, bb);
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
        addMovesFromBitboardSingle(moveList, moves, i, bb);
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
        addMovesFromBitboardSingle(moveList, moves, i, bb);
    }
}

void Board::genPawnMovesWhite(MoveList& moveList) const {
    const U64& pawns = boards[(int)BitBoards::PW];
    U64 pawnMoves;
    U64 empty = ~_occupied;
    // queenwards capture
    pawnMoves = (pawns << 7) & ~FILE_H & ~RANK_8 & (_blackPieces | enpassantTarget);
    addMovesFromBitboardParallel(moveList, pawnMoves & ~enpassantTarget, 7, BitBoards::PW, MoveTypes::Normal);
    addMovesFromBitboardParallel(moveList, pawnMoves & enpassantTarget, 7, BitBoards::PW, MoveTypes::EnpasQueen);
    // kingwards capture
    pawnMoves = (pawns << 9) & ~FILE_A & ~RANK_8 & (_blackPieces | enpassantTarget);
    addMovesFromBitboardParallel(moveList, pawnMoves & ~enpassantTarget, 9, BitBoards::PW, MoveTypes::Normal);
    addMovesFromBitboardParallel(moveList, pawnMoves & enpassantTarget, 9, BitBoards::PW, MoveTypes::EnpasKing);
    // Forward one
    pawnMoves = (pawns << 8) & ~RANK_8 & empty;
    addMovesFromBitboardParallel(moveList, pawnMoves, 8, BitBoards::PW, MoveTypes::Normal);
    // Forward two
    pawnMoves = (((pawns << 8) & empty) << 8) & WHITE_SIDE & empty;
    addMovesFromBitboardParallel(moveList, pawnMoves, 16, BitBoards::PW, MoveTypes::PawnDouble);  // add move type

    // Promote diag left
    pawnMoves = (pawns << 7) & ~FILE_H & RANK_8 & _blackPieces;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, 7, BitBoards::PW);
    // Promote diag right
    pawnMoves = (pawns << 9) & ~FILE_A & RANK_8 & _blackPieces;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, 9, BitBoards::PW);
    // Promote forward
    pawnMoves = (pawns << 8) & RANK_8 & empty;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, 8, BitBoards::PW);
}

void Board::genPawnMovesBlack(MoveList& moveList) const {
    const U64& pawns = boards[(int)BitBoards::PB];
    U64 pawnMoves;
    U64 empty = ~_occupied;
    // kingwards
    pawnMoves = (pawns >> 7) & ~FILE_A & ~RANK_1 & (_whitePieces | enpassantTarget);
    addMovesFromBitboardParallel(moveList, pawnMoves & ~enpassantTarget, -7, BitBoards::PB, MoveTypes::Normal);
    addMovesFromBitboardParallel(moveList, pawnMoves & enpassantTarget, -7, BitBoards::PB, MoveTypes::EnpasKing);
    // queenwards
    pawnMoves = (pawns >> 9) & ~FILE_H & ~RANK_1 & (_whitePieces | enpassantTarget);
    addMovesFromBitboardParallel(moveList, pawnMoves & ~enpassantTarget, -9, BitBoards::PB, MoveTypes::Normal);
    addMovesFromBitboardParallel(moveList, pawnMoves & enpassantTarget, -9, BitBoards::PB, MoveTypes::EnpasQueen);
    // Forward one
    pawnMoves = (pawns >> 8) & ~RANK_1 & empty;
    addMovesFromBitboardParallel(moveList, pawnMoves, -8, BitBoards::PB, MoveTypes::Normal);
    // Forward two
    pawnMoves = (((pawns >> 8) & empty) >> 8) & BLACK_SIDE & empty;
    addMovesFromBitboardParallel(moveList, pawnMoves, -16, BitBoards::PB, MoveTypes::PawnDouble);  // add move type

    // Promote diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & RANK_1 & _whitePieces;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, -7, BitBoards::PB);
    // Promote diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & RANK_1 & _whitePieces;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, -9, BitBoards::PB);
    // Promote forward
    pawnMoves = (pawns >> 8) & RANK_1 & empty;
    addMovesFromBitboardParallelPromote(moveList, pawnMoves, -8, BitBoards::PB);
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

void Board::addMovesFromBitboardSingle(MoveList& moves, U64 destinations, int position, BitBoards bb) const {
    int i = 0;
    while (destinations) {
        i = trailingZeros(destinations);
        U64 hot = 1ULL << i;
        destinations ^= hot;  // unset this bit
        CaptureType cap = hot & _occupied ? CaptureType::Capture : CaptureType::NonCapture;
        moves.add(GenMove(position, i, MovePromotions::None, bb, MoveTypes::Normal, cap));
    }
}

void Board::addMovesFromBitboardParallelPromote(MoveList& moves, U64 destinations, int offset, BitBoards bb) const {
    int i = 0;
    while (destinations) {
        i = trailingZeros(destinations);
        U64 hot = 1ULL << i;
        destinations ^= hot;  // unset this bit
        CaptureType cap = hot & _occupied ? CaptureType::Capture : CaptureType::NonCapture;
        moves.add(GenMove(i - offset, i, MovePromotions::Q, bb, MoveTypes::Promote, cap));
        moves.add(GenMove(i - offset, i, MovePromotions::R, bb, MoveTypes::Promote, cap));
        moves.add(GenMove(i - offset, i, MovePromotions::N, bb, MoveTypes::Promote, cap));
        moves.add(GenMove(i - offset, i, MovePromotions::B, bb, MoveTypes::Promote, cap));
    }
}

void Board::addMovesFromBitboardParallel(MoveList& moves, U64 destinations, int offset, BitBoards bb, MoveTypes type) const {
    int i = 0;
    while (destinations) {
        i = trailingZeros(destinations);
        U64 hot = 1ULL << i;
        destinations ^= hot;  // unset this bit
        CaptureType cap = hot & _occupied ? CaptureType::Capture : CaptureType::NonCapture;
        moves.add(GenMove(i - offset, i, MovePromotions::None, bb, type, cap));
    }
}
