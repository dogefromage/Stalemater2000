#pragma once
#include <thread>
#include <chrono>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include "Board.h"
#include "Debug.h"
#include "Evaluation.h"
#include "Position.h"

#define U64 unsigned long long

class Computer
{
public:
    static bool Working;
    static std::unordered_map<U64, int> BestMoveTable;
    static std::unordered_map<U64, Position> PositionTable;

    static std::string GetMessage();

    static void AddMessage(std::string msg);

    static void ChooseMove(Board board, int maxDepth);

    static void PerftAnalysis(Board board, int depth);
    
    static void ZobristTest(Board board, int depth);

private:
    static std::queue<std::string> messages;

    static int randomMove(Board& board);

    static Score search(const Board& board, int currDepth, int maxDepth, Score alpha, Score beta);

    static Score quiescence(const Board& board, Score alpha, Score beta);

    static void perftThread(Board board, int depth, int index);

    static unsigned long long perft(Board& board, int depth);
};

#undef U64