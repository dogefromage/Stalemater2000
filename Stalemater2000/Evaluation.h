#pragma once
#include "Board.h"

#define U64 unsigned long long

#define Score short
#define SCORE_NONE 32767

// SOME VALUES AND TABLES //https://www.chessprogramming.org/Simplified_Evaluation_Function

constexpr Score PIECE_VALUES[] =
{
    100, 500, 320, 330, 900, 2000
};

constexpr Score PIECE_SQUARE_TABLE[] =
{
    // pawn
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 25, 25,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
    // rook
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
    // knight
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 15, 15, 15, 15,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50,
    // bishop
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20,
    //queen
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20,
    //king middle game
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20,
    // king end game
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

constexpr U64 KING_ZONE_SPAN = 0x1F1F1F1F1F;
constexpr int KING_ZONE_OFFSET = 18;

// source: https://www.chessprogramming.org/King_Safety
constexpr Score KING_ATTACKER_DANGER[] =
{
    //20 for a knight, 20 for a bishop, 40 for a rook and 80 for a queen
    5, 40, 20, 20, 80, 5
};

constexpr Score KING_NUMBER_ATTACKERS_WEIGHT[] =
{
    0,  // 0
    0,  // 1
    50, // 2
    75, // ...
    88,
    94,
    97,
    99, // 7
    99, 99, 99, 99, 99, 99, 99, 99, 99 // prevent out of range
};

struct Evaluation
{
    static Score evaluate(const Board& board);

private:
    static Score evaluateBalance(const Board& board);

    static Score evaluatePiecePositions(const Board& board, int endgameFactor);

    static Score evaluateMobility(const Board& board);

    static Score evaluatePawnStructure(const Board& board, int endgameFactor, bool isWhite);

    static Score evaluateKingSafety(const Board& board, bool isWhite);
};

#undef U64