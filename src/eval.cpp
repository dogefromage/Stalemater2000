#include "eval.h"

#include "bitmath.h"
#include "board.h"
#include "nnue.h"

Score evaluateBalance(const Board& board) {
    Score eval = 0;
    for (int i = 0; i < 5; i++) {
        Score balance = countBits(board.getBoard((BitBoards)i)) - countBits(board.getBoard((BitBoards)(i + 6)));
        eval += PIECE_VALUES[i] * balance;
    }
    return eval;
}

Score evaluatePiecePositions(const Board& board, float endgameFactor) {
    Score eval = 0;
    // white
    for (int b = 0; b < 6; b++) {
        // pull bitboard
        U64 bb = board.getBoard((BitBoards)b);
        if (b == 5) {
            int tableOffset = b * 64 + 63;
            int eval1, eval2;
            while (bb) {
                int i = trailingZeros(bb);
                eval1 = PIECE_SQUARE_SCORE[tableOffset - i];
                eval2 = PIECE_SQUARE_SCORE[tableOffset + 64 - i];  // walk through array reverse bc. white
                eval += (Score)(eval1 * (1.f - endgameFactor) + eval2 * endgameFactor);
                bb ^= 1ULL << i;  // unset bit
            }
        } else {
            int tableOffset = b * 64 + 63;
            while (bb) {
                int i = trailingZeros(bb);
                eval += PIECE_SQUARE_SCORE[tableOffset - i];  // walk through array reverse bc. white
                bb ^= 1ULL << i;                              // unset bit
            }
        }
    }

    for (int b = 0; b < 6; b++) {
        // pull bitboard
        U64 bb = board.getBoard((BitBoards)(b + 6));

        if (b == 5) {
            int tableOffset = b * 64;
            int eval1, eval2;
            while (bb) {
                int i = trailingZeros(bb);
                eval1 = PIECE_SQUARE_SCORE[tableOffset + i];
                eval2 = PIECE_SQUARE_SCORE[tableOffset + 64 + i];  // walk through array reverse bc. white
                eval -= (int)(eval1 * (1.f - endgameFactor) + eval2 * endgameFactor);
                bb ^= 1ULL << i;  // unset bit
            }
        } else {
            int tableOffset = b * 64;
            while (bb) {
                int i = trailingZeros(bb);
                eval -= PIECE_SQUARE_SCORE[tableOffset + i];  // walk through array reverse bc. white
                bb ^= 1ULL << i;                              // unset bit
            }
        }
    }
    return eval;
}

Score evaluateMobility(Board& board) {
    Score mob = countBits(board.getUnsafeForWhite());
    mob -= countBits(board.getUnsafeForBlack());
    return mob;
}

Score evaluatePawnStructure(Board& board, bool isWhite) {
    U64 pawns = board.getBoard(isWhite ? BitBoards::PW : BitBoards::PB);

    // doubled
    int doubled = 0;
    for (int f = 0; f < 8; f++) {
        int n = 0;
        U64 pawnsOnFile = pawns & FILE_MASKS[f];
        while (pawnsOnFile) {
            popLSB(pawnsOnFile);
            n++;
        }
        if (n > 1) {
            doubled += n - 1;
        }
    }

    // blocked
    U64 infrontOfPawns = isWhite ? (pawns << 8) : (pawns >> 8);
    int blocked = countBits(infrontOfPawns & board.getOccupied());

    // isolated (not guarded)
    U64 unsafeForOther = isWhite ? board.getUnsafeForBlack() : board.getUnsafeForWhite();
    U64 undefendedPawns = pawns & ~unsafeForOther;
    int isolated = countBits(undefendedPawns);

    // chained
    int chained = 0;
    if (isWhite) {
        // counts all occurences of pawns which support other pawns diagonally
        chained += countBits(pawns & ((pawns >> 7) & ~FILE_A));
        chained += countBits(pawns & ((pawns >> 9) & ~FILE_H));
    } else {
        chained += countBits(pawns & ((pawns << 7) & ~FILE_H));
        chained += countBits(pawns & ((pawns << 9) & ~FILE_A));
    }

    Score totalPawnsEval = chained - doubled - blocked - isolated;

    return totalPawnsEval;
}

Score evaluateKingSafety(Board& board, bool isWhite, float endgameFactor) {
    U64 king = board.getBoard(isWhite ? BitBoards::KW : BitBoards::KB);

    int kingIndex = trailingZeros(king);
    // directly stolen from horse move generation
    int offset = kingIndex - KING_ZONE_OFFSET;
    U64 kingZone = (offset > 0) ? (KING_ZONE_SPAN << offset) : (KING_ZONE_SPAN >> -offset);  // weird stuff happens when shifting by neg number
    if (kingIndex % 8 < 4) {
        kingZone &= ~FILE_GH;
    } else {
        kingZone &= ~FILE_AB;
    }

    // direct danger -> pieces inside kings zone
    int directDanger = 0;
    int numAttackers = 0;
    for (int b = 0; b < 6; b++) {
        int boardIndex = isWhite ? (b + 6) : b;
        U64 attackers = kingZone & board.getBoard((BitBoards)boardIndex);
        while (attackers) {
            popLSB(attackers);
            numAttackers++;
            directDanger += KING_ATTACKER_DANGER[b];
        }
    }
    directDanger = directDanger * KING_NUMBER_ATTACKERS_WEIGHT[numAttackers] / 50;  // apply weight

    // indirect danger -> unsafe squares
    U64 unsafeBoard = isWhite ? board.getUnsafeForWhite() : board.getUnsafeForBlack();
    int unsafeSquares = countBits(unsafeBoard & kingZone);

    int kingCenterPosition = 0;
    for (int i = 0; i < 4; i++) {
        if (king & RING_MASK[i]) {
            kingCenterPosition += i;
            break;
        }
    }
    kingCenterPosition = (int)(kingCenterPosition * endgameFactor * endgameFactor * 5);

    Score totalKingSafety =
        -directDanger * 2 - unsafeSquares - kingCenterPosition;

    return totalKingSafety;
}

Score evaluate_qualitative(Board& board) {
    Score eval = 0;

    int totalPopulation = countBits(board.getOccupied());
    float endgameFactor = 1.0f - (totalPopulation / 32.0f);

    // balance
    eval += evaluateBalance(board);

    // piece positions
    eval += 2 * evaluatePiecePositions(board, endgameFactor);

    // Mobility
    eval += 2 * evaluateMobility(board);

    // pawns
    Score pawns = evaluatePawnStructure(board, true) - evaluatePawnStructure(board, false);
    eval += 6 * pawns;

    // king safety
    eval += evaluateKingSafety(board, endgameFactor, true);
    eval -= evaluateKingSafety(board, endgameFactor, false);

    /*
    // has castled
    if (board.hasCastled & WHITE_HAS_CASTLED) eval += 40;
    if (board.hasCastled & BLACK_HAS_CASTLED) eval -= 40;

    // can castle
    if (board.castling & CASTLE_KW) eval += 10;
    if (board.castling & CASTLE_QW) eval += 8;
    if (board.castling & CASTLE_KB) eval -= 10;
    if (board.castling & CASTLE_QB) eval -= 8;
    */

    if (board.getSideToMove() == Side::Black) {
        eval = -eval;
    }

    return eval;
}
