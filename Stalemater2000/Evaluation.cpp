#include "Evaluation.h"

#define U64 unsigned long long

Score Evaluation::evaluate(const Board& board)
{
    Score eval = 0;
    int totalPopulation = countBits(board.Occupied);
    int endgameFactor = 32 - totalPopulation; // 0 ~ 30

    // balance
    eval += evaluateBalance(board);
    // piece positions
    eval += evaluatePiecePositions(board, endgameFactor);
    // Mobility
    eval += evaluateMobility(board);
    //// pawns
    eval += evaluatePawnStructure(board, endgameFactor, true);
    eval -= evaluatePawnStructure(board, endgameFactor, false);
    //// king safety
    eval += evaluateKingSafety(board, true);
    eval -= evaluateKingSafety(board, false);

    if (board.HasCastled & WHITE_HAS_CASTLED) eval += 40;
    if (board.Castling & CASTLE_KW) eval += 12;
    if (board.Castling & CASTLE_QW) eval += 8;
    if (board.HasCastled & BLACK_HAS_CASTLED) eval -= 40;
    if (board.Castling & CASTLE_KB) eval -= 12;
    if (board.Castling & CASTLE_QB) eval -= 8;

    return eval;
}
    
Score Evaluation::evaluateBalance(const Board& board)
{
    Score eval = 0;
    for (int i = 0; i < 5; i++)
    {
        Score balance = countBits(board.BitBoards[i]) - countBits(board.BitBoards[i + 6]);
        eval += PIECE_VALUES[i] * balance;
    }
    return eval;
}

Score Evaluation::evaluatePiecePositions(const Board& board, int endgameFactor)
{
    Score eval = 0;

    // white
    for (int b = 0; b < 6; b++)
    {
        // pull bitboard
        U64 bb = board.BitBoards[b];
        //if (endgameFactor > 18 && b == 5)
        //    b++;
        int tableOffset = b * 64 + 63;
        int i = 0;
        while (bb)
        {
            i = trailingZeros(bb);
            eval += PIECE_SQUARE_TABLE[tableOffset - i]; // walk through array reverse bc. white
            bb ^= 1ULL << i; // unset bit
        }
    }

    for (int b = 0; b < 6; b++)
    {
        // pull bitboard
        U64 bb = board.BitBoards[b + 6];

        //if (endgameFactor > 18 && b == 5)
        //    b++;
        int tableOffset = b * 64;
        int i = 0;
        while (bb)
        {
            i = trailingZeros(bb);
            eval -= PIECE_SQUARE_TABLE[tableOffset + i]; // normal
            bb ^= 1ULL << i; // unset bit
        }
    }
    
    return eval * 2;
}

Score Evaluation::evaluateMobility(const Board& board)
{
    Score mob = countBits(board.UnsafeForWhite);
    mob -= countBits(board.UnsafeForBlack);
    return (10 * mob);
}

Score Evaluation::evaluatePawnStructure(const Board& board, int endgameFactor, bool isWhite)
{
    const U64& pawns = isWhite ? board.BitBoards[PW] : board.BitBoards[PB];
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
    blocked = countBits(p & board.Occupied);

    // isolated (not guarded)
    p = pawns & ~(isWhite ? board.UnsafeForBlack : board.UnsafeForWhite);
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
        +   chained
        -   doubled
        -   blocked
        -   isolated;
    
    totalPawnsEval *= endgameFactor + 5;
    
    return totalPawnsEval;
}

Score Evaluation::evaluateKingSafety(const Board& board, bool isWhite)
{
    const U64& king = isWhite ? board.BitBoards[KW] : board.BitBoards[KB];

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
            attackers = kingZone & board.BitBoards[b + 6];
        else
            attackers = kingZone & board.BitBoards[b];

        while (attackers)
        {
            popLSB(attackers);
            numAttackers++;
            directDanger += KING_ATTACKER_DANGER[b];
        }
    }
    directDanger = directDanger * KING_NUMBER_ATTACKERS_WEIGHT[numAttackers] / 100; // apply weight

    // indirect danger -> unsafe squares
    U64 unsafeBoard = isWhite ? board.UnsafeForWhite : board.UnsafeForBlack;
    int unsafeSquares = countBits(unsafeBoard & kingZone);

    Score totalKingSafety = -directDanger;

    return totalKingSafety;
}

#undef U64