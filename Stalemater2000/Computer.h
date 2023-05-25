#pragma once
#include <array>
#include <vector>
#include "HashBoard.h"
#include "Evaluation.h"

typedef struct Position {
    Score score;
    short depth;
} Position;

enum class ComputerTests {
    Perft,
    Zobrist,
};
enum class LongSearchParameters {
    wtime, btime, winc, binc, movestogo, depth, nodes, mate, movetime,
    SIZE,
};
enum class BoolSearchParameters {
    infinite, ponder,
    SIZE,
};

class Computer {
public:
    void stop();
    bool isWorking();

    void launchTest(ComputerTests testType, int depth);
    void launchSearch(
        std::array<std::int64_t, (size_t)LongSearchParameters::SIZE> longParams,
        std::array<bool, (size_t)BoolSearchParameters::SIZE> boolParams,
        std::vector<LanMove> searchmoves
    );

    // static bool Working;

private:
    /*
    static std::unordered_map<U64, int> BestMoveTable;
    static std::unordered_map<U64, Position> PositionTable;

    static void AddMessage(std::string msg);

    static void ChooseMove(const Board& board, int maxDepth);

    static void PerftAnalysis(Board board, int depth);
    
    static void ZobristTest(Board board, int depth);

    static std::queue<std::string> messages;

    static int randomMove(Board& board);

    static Score search(const Board& board, int remainingDepth, Score alpha, Score beta);

    static Score quiescence(const Board& board, Score alpha, Score beta);

    static void perftThread(Board board, int depth, int index);

    static uint64_t perft(Board& board, int depth);
    */
};

