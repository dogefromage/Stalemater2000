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

struct SearchNode {
    Score score;
    short depth;
    GenMove pv;
};

class ComputerSearchTask {
   public:
    Position rootPosition;
    SearchParams params;

    long currNodesSearched, prevTotalNodesSearched;
    std::chrono::_V2::system_clock::time_point lastTime, startTime;
    int iterativeDepth;

    ComputerSearchTask(Position rootPosition) {
        this->rootPosition = rootPosition;
        currNodesSearched = prevTotalNodesSearched = 0;
        lastTime = startTime = std::chrono::high_resolution_clock::now();
    }
};

class Computer {
   public:
    // needs to hold output lock to access info and bestmove
    std::mutex outputLock;
    std::vector<ComputerInfo> infoBuffer = {};
    std::unique_ptr<LanMove> bestMove = NULL;

    std::atomic<bool> isWorking = false;

    void stopWorking();
    void launchTest(Position root, ComputerTests testType, int depth);
    void launchSearch(ComputerSearchTask* task);

   private:
    std::unordered_map<U64, SearchNode> searchTable;

    long perft(Position& curr, int depth);
    void launchPerft(Position& root, int depth);
    void launchZobrist(Position& root, int depth);
    bool mustStopSearching();
    Score search(Position curr, int remainingDepth, Score alpha, Score beta);
    std::string getPvList(Position board);
    void generateComputerInfo();
    
    ComputerSearchTask* task;
};