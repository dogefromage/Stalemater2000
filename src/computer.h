#pragma once
#include <array>
#include <vector>

#include "position.h"
#include "board.h"
#include "eval.h"

// typedef struct Node {
//     Score score;
//     short depth;
// } Node;

enum class ComputerTests {
    Perft,
    Zobrist,
};
enum class LongSearchParameters {
    wtime,
    btime,
    winc,
    binc,
    movestogo,
    depth,
    nodes,
    mate,
    movetime,
    SIZE,
};
enum class BoolSearchParameters {
    infinite,
    ponder,
    SIZE,
};

void stopComputer();
bool isComputerWorking();

void launchTest(Position root, ComputerTests testType, int depth);
void launchSearch(
    Position root,
    std::array<std::int64_t, (size_t)LongSearchParameters::SIZE> longParams,
    std::array<bool, (size_t)BoolSearchParameters::SIZE> boolParams,
    std::vector<LanMove> searchmoves);

//    private:
//     /*
//     static std::unordered_map<U64, int> BestMoveTable;
//     static std::unordered_map<U64, Position> PositionTable;

//     static void AddMessage(std::string msg);

//     static void ChooseMove(const Board& board, int maxDepth);

//     static void PerftAnalysis(Board board, int depth);

//     static void ZobristTest(Board board, int depth);

//     static std::queue<std::string> messages;

//     static int randomMove(Board& board);

//     static Score search(const Board& board, int remainingDepth, Score alpha, Score beta);

//     static Score quiescence(const Board& board, Score alpha, Score beta);

//     static void perftThread(Board board, int depth, int index);

//     static uint64_t perft(Board& board, int depth);
//     */
// };
