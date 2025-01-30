#pragma once
#include <array>
#include <vector>

#include "position.h"
#include "board.h"
#include "eval.h"
#include <mutex>
#include <memory>

enum class ComputerTests {
    Perft,
    Zobrist,
};

struct SearchParams {
    long wtime, btime, winc, binc, movestogo, depth, nodes, mate, movetime;
    bool infinite, ponder;
};

long* getSearchParamLongField(SearchParams* searchParams, std::string& name);
bool* getSearchParamBoolField(SearchParams* searchParams, std::string& name);

void stopComputer();
bool isComputerWorking();

void launchTest(Position root, ComputerTests testType, int depth);
void launchSearch(
    Position root,
    SearchParams params,
    std::vector<LanMove> searchmoves);

struct ComputerInfo {
    long depth, score, nodes, nps;
    std::string pv;
};

extern std::mutex computerOutputLock;
// needs to hold lock to access info and bestmove
extern std::vector<ComputerInfo> infoBuffer;
extern std::unique_ptr<LanMove> bestMove;
