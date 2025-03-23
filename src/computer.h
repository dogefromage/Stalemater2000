#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "board.h"
#include "eval.h"
#include "position.h"
#include "nnue.h"

enum class ComputerTests {
    Perft,
    Zobrist,
};

struct StringComparator {
    bool operator()(char const* a, char const* b) const {
        return std::strcmp(a, b) < 0;
    }
};

struct SearchParams {
    std::map<const char*, long, StringComparator> attributes;
    std::vector<LanMove> searchmoves;
    
    long getLongField(const char* key) const;
    bool getBoolField(const char* key) const;
    // long wtime, btime, winc, binc, movestogo, depth, nodes, mate, movetime;
    // bool infinite, ponder;
};

struct ComputerInfo {
    long depth, score, nodes, nps;
    std::string pv;
};

// enum class SearchNodeType {
//     Branch,
//     Terminal,
// };

struct SearchNode {
    GenMove pv;
    Score score;
    short knownDepth;
};

class ComputerSearchTask {
   public:
    Position rootPosition;
    SearchParams params;

    long currNodesSearched, prevTotalNodesSearched;
    std::chrono::_V2::system_clock::time_point lastTime, startTime;
    int iterativeDepth;

    ComputerSearchTask() {}

    ComputerSearchTask(Position rootPosition) {
        this->rootPosition = rootPosition;
        currNodesSearched = prevTotalNodesSearched = 0;
        lastTime = startTime = std::chrono::high_resolution_clock::now();
    }
};

class Computer {
   public:
    std::atomic<bool> isWorking = false;

    ComputerSearchTask task;

    // needs to hold output lock to access info and bestmove
    std::mutex outputLock;
    std::vector<ComputerInfo> infoBuffer = {};
    std::unique_ptr<LanMove> bestMove = NULL;

    void stopWorking();
    void launchTest(Position root, ComputerTests testType, int depth);
    void launchSearch();

   private:
    std::unordered_map<U64, SearchNode> searchTable;
    AccumulatorStack accumulators;

    Score evaluate_relative(Board& board, int depth);
    long perft(Position& curr, int depth);
    void launchPerft(Position& root, int depth);
    void launchZobrist(Position& root, int depth);
    bool mustStopSearching();
    Score quiescence(Position& curr, int currentDepth, Score alpha, Score beta);
    Score search(Position& curr, int currentDepth, Score alpha, Score beta);
    std::string getPvList(Position board);
    void generateComputerInfo();
    
};