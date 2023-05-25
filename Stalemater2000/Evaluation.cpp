#include "Evaluation.h"
#include "bitmath.h"
#include "HashBoard.h"

Score evaluateBalance(const HashBoard& board) {
    Score eval = 0;
    for (int i = 0; i < 5; i++) {
        Score balance = countBits(board.getBoard((BitBoards)i)) 
                      - countBits(board.getBoard((BitBoards)(i + 6)));
        eval += PIECE_VALUES[i] * balance;
    }
    return eval;
}

/*
Score evaluatePiecePositions(const Board& board, float endgameFactor)
{
    Score eval = 0;

    // white
    int i = 0;
    for (int b = 0; b < 6; b++)
    {
        // pull bitboard
        U64 bb = board.boards[b];
        if (b == 5)
        {
            int tableOffset = b * 64 + 63;
            int eval1, eval2;
            while (bb)
            {
                i = trailingZeros(bb);
                eval1 = PIECE_SQUARE_TABLE[tableOffset - i];
                eval2 = PIECE_SQUARE_TABLE[tableOffset + 64 - i]; // walk through array reverse bc. white
                eval += (int)(eval1 * (1.f - endgameFactor) + eval2 * endgameFactor);
                bb ^= 1ULL << i; // unset bit
            }
        }
        else
        {
            int tableOffset = b * 64 + 63;
            while (bb)
            {
                i = trailingZeros(bb);
                eval += PIECE_SQUARE_TABLE[tableOffset - i]; // walk through array reverse bc. white
                bb ^= 1ULL << i; // unset bit
            }
        }
    }

    for (int b = 0; b < 6; b++)
    {
        // pull bitboard
        U64 bb = board.boards[b + 6];

        if (b == 5)
        {
            int tableOffset = b * 64;
            int eval1, eval2;
            while (bb)
            {
                i = trailingZeros(bb);
                eval1 = PIECE_SQUARE_TABLE[tableOffset + i];
                eval2 = PIECE_SQUARE_TABLE[tableOffset + 64 + i]; // walk through array reverse bc. white
                eval -= (int)(eval1 * (1.f - endgameFactor) + eval2 * endgameFactor);
                bb ^= 1ULL << i; // unset bit
            }
        }
        else
        {
            int tableOffset = b * 64;
            while (bb)
            {
                i = trailingZeros(bb);
                eval -= PIECE_SQUARE_TABLE[tableOffset + i]; // walk through array reverse bc. white
                bb ^= 1ULL << i; // unset bit
            }
        }
    }

    return eval / 2;
}

Score evaluateMobility(const Board& board)
{
    Score mob = countBits(board.unsafeForWhite);
    mob -= countBits(board.unsafeForBlack);
    return (3 * mob);
}

Score evaluatePawnStructure(const Board& board, float endgameFactor, bool isWhite)
{
    const U64& pawns = isWhite ? board.boards[PW] : board.boards[PB];
    U64 p;

    int doubled, blocked, isolated, chained;

    // doubled
    doubled = 0;
    int n;
    for (int f = 0; f < 8; f++)
    {
        n = 0;
        p = pawns & FILE_MASKS[f];
        while (p)
        {
            popLSB(p);
            n++;
        }

        if (n > 1)
        {
            doubled += n - 1;
        }
    }

    // blocked
    p = isWhite ? (pawns << 8) : (pawns >> 8);
    blocked = countBits(p & board.occupied);

    // isolated (not guarded)
    p = pawns & ~(isWhite ? board.unsafeForBlack : board.unsafeForWhite);
    isolated = countBits(p);

    // chained
    chained = 0;
    if (isWhite)
    {
        // counts all occurences of pawns which support other pawns diagonally
        chained += countBits(pawns & ((pawns >> 7) & ~FILE_A));
        chained += countBits(pawns & ((pawns >> 9) & ~FILE_H));
    }
    else
    {
        chained += countBits(pawns & ((pawns << 7) & ~FILE_H));
        chained += countBits(pawns & ((pawns << 9) & ~FILE_A));
    }
    
    Score totalPawnsEval =
        + chained
        - doubled
        - blocked
        - isolated;
    
    return totalPawnsEval;
}

Score evaluateKingSafety(const Board& board, bool isWhite, float endgameFactor)
{
    const U64& king = isWhite ? board.boards[KW] : board.boards[KB];

    int kingIndex = trailingZeros(king);
    // directly stolen from horse move generation
    int offset = kingIndex - KING_ZONE_OFFSET;
    U64 kingZone = (offset > 0) ? (KING_ZONE_SPAN << offset) : (KING_ZONE_SPAN >> -offset); // weird stuff happens when shifting by neg number
    if (kingIndex % 8 < 4)  kingZone &= ~FILE_GH;
    else                    kingZone &= ~FILE_AB;

    // direct danger -> pieces inside kings zone
    int directDanger = 0;
    int numAttackers = 0;
    U64 attackers;
    for (int b = 0; b < 6; b++)
    {
        if (isWhite)
            attackers = kingZone & board.boards[b + 6];
        else
            attackers = kingZone & board.boards[b];

        while (attackers)
        {
            popLSB(attackers);
            numAttackers++;
            directDanger += KING_ATTACKER_DANGER[b];
        }
    }
    directDanger = directDanger * KING_NUMBER_ATTACKERS_WEIGHT[numAttackers] / 100; // apply weight

    // indirect danger -> unsafe squares
    U64 unsafeBoard = isWhite ? board.unsafeForWhite : board.unsafeForBlack;
    int unsafeSquares = countBits(unsafeBoard & kingZone);

    int kingCenterPosition = 0;
    for (int i = 0; i < 4; i++)
    {
        if (king & RING_MASK[i])
        {
            kingCenterPosition += i;
            break;
        }
    }
    kingCenterPosition = (int)(kingCenterPosition * endgameFactor * endgameFactor * 5);

    Score totalKingSafety = 
        -   directDanger * 2
        -   unsafeSquares
        -   kingCenterPosition;

    return totalKingSafety;
}
*/

Score Evaluation::evaluate(const HashBoard& board)
{
    Score eval = 0;

    /*
    int totalPopulation = countBits(board.occupied);
    float endgameFactor = 1.0f - (totalPopulation / 32.0f);
    */

    // balance
    eval += evaluateBalance(board);

    /*


    // piece positions
    eval += evaluatePiecePositions(board, endgameFactor);
    //Mobility
    //eval += evaluateMobility(board);
    // pawns
    eval += evaluatePawnStructure(board, endgameFactor, true);
    eval -= evaluatePawnStructure(board, endgameFactor, false);
    // king safety
    //eval += evaluateKingSafety(board, endgameFactor, true);
    //eval -= evaluateKingSafety(board, endgameFactor, false);

    
    // has castled
    if (board.hasCastled & WHITE_HAS_CASTLED) eval += 40;
    if (board.hasCastled & BLACK_HAS_CASTLED) eval -= 40;

    // can castle
    if (board.castling & CASTLE_KW) eval += 10;
    if (board.castling & CASTLE_QW) eval += 8;
    if (board.castling & CASTLE_KB) eval -= 10;
    if (board.castling & CASTLE_QB) eval -= 8;
    */

    return eval;
}
