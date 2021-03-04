#pragma once
#include <thread>
#include <chrono>
#include <time.h>
#include <unordered_map>

#include "Board.h"
#include "Debug.h"
#include "Evaluation.h"
#include "Position.h"

#define U64 unsigned long long

extern std::string messageOut;

class Computer
{
public:
    static bool Working;
    static std::unordered_map<U64, Position> PositionTable;

    static void Init();

    static void ChooseMove(Board board, int maxDepth);

    static void PerftAnalysis(Board board, int depth);
    
    static void ZobristTest(Board board, int depth);

private:
    static int randomMove(Board& board);

    static Score search(const Board& board, int depth, Score alpha, Score beta);

    static void perftThread(Board board, int depth, int index);

    static unsigned long long perft(Board& board, int depth);
};

#undef U64