#include "computer.h"

#include <set>
#include <vector>

#include "position.h"

long Computer::perft(Position& curr, int depth) {
    if (depth == 0) {
        return 1;
    }

    long count = 0;
    MoveList pseudoMoves;
    curr.board.generatePseudoMoves(pseudoMoves);

    for (const GenMove& m : pseudoMoves) {
        Position next(curr);
        if (!isWorking) {
            break;
        }
        next.movePseudoInPlace(m);
        if (!next.board.isLegal()) {
            continue;  // illegal move
        }
        count += perft(next, depth - 1);  // recurse
    }
    return count;
}

void Computer::launchPerft(Position& root, int depth) {
    isWorking = true;

    long total = 0;

    MoveList pseudoMoves;
    root.board.generatePseudoMoves(pseudoMoves);

    for (const GenMove& m : pseudoMoves) {
        Position next(root);
        if (!isWorking) {
            break;
        }
        next.movePseudoInPlace(m);
        if (!next.board.isLegal()) {
            continue;  // illegal move
        }
        long moveScore = perft(next, depth - 1);  // recurse
        total += moveScore;

        std::cout << m.toLanMove().toString() << ": " << moveScore << std::endl;
    }

    std::cout << "Total: " << total << std::endl;

    isWorking = false;
}

void Computer::launchZobrist(Position& root, int depth) {
    (void)root;
    (void)depth;
    std::cerr << "Implement zobrist" << std::endl;
}

void Computer::stopWorking() {
    if (!isWorking) {
        printf("computer is not working\n");
    }
    isWorking = false;
}

void Computer::launchTest(Position root, ComputerTests testType, int depth) {
    if (isWorking) {
        std::cerr << "A task is already running." << std::endl;
        return;
    }
    switch (testType) {
        case ComputerTests::Perft:
            launchPerft(root, depth);
            break;
        case ComputerTests::Zobrist:
            launchZobrist(root, depth);
            break;
    }
}

// time managment
bool Computer::mustStopSearching() {
    if (task.params.getBoolField("infinite")) {
        return false;  // never stop here
    }

    // depth <x>
    long depthParam = task.params.getLongField("depth");
    if (depthParam > 0 && task.iterativeDepth > depthParam) {
        return true;
    }

    // nodes <x>, mate <x>,
    // TODO

    auto curr = std::chrono::high_resolution_clock::now();
    long totalMillis = std::chrono::duration_cast<std::chrono::milliseconds>(curr - task.startTime).count();

    // movetime <ms>
    long movetimeParam = task.params.getLongField("movetime");
    if (movetimeParam > 0 && totalMillis >= movetimeParam) {
        return true;
    }

    long fullMovesCount = task.rootPosition.fullMovesCount;

    float wtime = (float)task.params.getLongField("wtime");
    float btime = (float)task.params.getLongField("btime");
    float winc = (float)task.params.getLongField("winc");
    float binc = (float)task.params.getLongField("binc");
    long movestogo = task.params.getLongField("movestogo");

    bool isWhite = task.rootPosition.board.getSideToMove() == Side::White;
    float remainingTime = isWhite ? wtime : btime;
    float increment = isWhite ? winc : binc;

    if (remainingTime > 0) {
        // do time management
        if (increment < 0) {
            increment = 0;
        }
        if (movestogo <= 0) {
            // sudden death
            movestogo = std::max(20L, 40L - fullMovesCount);
        }

        float availableTime = remainingTime + increment * movestogo;
        float moveLimitMillis = availableTime / movestogo;

        // limit time in opening
        float minOpeningFactor = 0.33;
        int openingFullMoves = 8;
        float openingFactor = std::max(minOpeningFactor, std::min(fullMovesCount / (float)openingFullMoves, 1.0f));

        moveLimitMillis *= openingFactor;

        // factor in some delay
        moveLimitMillis = 0.8 * (moveLimitMillis - 500);

        if (totalMillis > moveLimitMillis) {
            return true;
        }
    }

    return false;
}

Score Computer::evaluate_relative(Board& board, int depth) {
    int32_t eval = accumulators.forward(depth, board.getSideToMove(), board.getOccupied());
    // Score eval = evaluate_qualitative(board);

    if (eval < -MAX_EVAL) {
        eval = -MAX_EVAL;
    } else if (eval > MAX_EVAL) {
        eval = MAX_EVAL;
    }

    return (Score)eval;
}

Score Computer::quiescence(Position& pos, int currentDepth, Score alpha, Score beta) {

    task.currNodesSearched++;
    // only rarely check if out of time
    if (task.currNodesSearched % 100000 == 0) {
        if (mustStopSearching()) {
            isWorking = false;
        }
    }

    if (!isWorking) {
        return alpha;
    }
    
    Score standPat = evaluate_relative(pos.board, currentDepth);

    if (standPat >= beta) {
        // can already be pruned here
        return standPat;
    }
    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList captures;
    pos.board.generatePseudoMoves(captures);
    // TODO: maybe consider adding pv but it may be slower here
    pos.board.orderAndFilterMoveList(captures, LanMove::NullMove(), true);

    for (GenMove& m : captures) {

        Position nextPos(pos);
        accumulators.markDirty(currentDepth + 1);
        nextPos.board.editRecorder = accumulators.getRecorder(currentDepth + 1);
        nextPos.movePseudoInPlace(m);
        nextPos.board.editRecorder = nullptr;

        if (!nextPos.board.isLegal()) {
            continue;
        }

        Score score = -quiescence(nextPos, currentDepth + 1, -beta, -alpha);

        if (score >= beta) {
            // prune branch
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Score Computer::search(Position& pos, int currentDepth, Score alpha, Score beta) {
    int remainingDepth = task.iterativeDepth - currentDepth;

    LanMove lastPv = LanMove::NullMove();
    auto boardEntry = searchTable.find(pos.board.getHash());
    if (boardEntry != searchTable.end()) {
        if (boardEntry->second.knownDepth >= remainingDepth) {
            // has already more knowledge over this node => skip
            return boardEntry->second.score;
        }
        // grab last pv
        lastPv = boardEntry->second.pv.toLanMove();
    }
    if (boardEntry != searchTable.end() &&
        boardEntry->second.knownDepth >= remainingDepth) {
        // has already more knowledge over this node => skip
        return boardEntry->second.score;
    }

    if (currentDepth >= task.iterativeDepth) {
        return quiescence(pos, currentDepth, alpha, beta);
        // return evaluate_relative(pos.board, accumulators, currentDepth);
    }

    task.currNodesSearched++;

    // only rarely check if out of time
    if (task.currNodesSearched % 100000 == 0) {
        if (mustStopSearching()) {
            isWorking = false;
        }
    }
    if (!isWorking) {
        return alpha;
    }

    Score bestScore = -SCORE_CHECKMATE + currentDepth;
    GenMove bestMove = GenMove::NullMove();
    MoveList moves;
    pos.board.generatePseudoMoves(moves);
    pos.board.orderAndFilterMoveList(moves, lastPv, false);

    for (GenMove& m : moves) {

        Position nextPos(pos);
        accumulators.markDirty(currentDepth + 1);
        nextPos.board.editRecorder = accumulators.getRecorder(currentDepth + 1);
        nextPos.movePseudoInPlace(m);
        nextPos.board.editRecorder = nullptr;

        if (!nextPos.board.isLegal()) {
            continue;
        }

        if (bestMove.isNullMove()) {
            bestMove = m;
        }

        Score score = -search(nextPos, currentDepth + 1, -beta, -alpha);

        if (score >= beta) {
            // prune branch
            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (!isWorking) {
        return alpha;
    }

    bool stalemate = false;  // add counting remaining pieces and threefold repetition

    if (bestMove.isNullMove()) {
        // no moves
        if (pos.board.getSideToMove() == Side::White) {
            if (!(pos.board.hasCheck(CheckFlags::WhiteInCheck))) {
                // white has no moves but is not in check
                stalemate = true;
            }
        } else if (!(pos.board.hasCheck(CheckFlags::BlackInCheck))) {
            stalemate = true;
        }
    }
    if (stalemate) {
        bestScore = 0;
    }

    searchTable[pos.board.getHash()] = { .pv = bestMove, .score = bestScore, .knownDepth = (short)remainingDepth };

    return bestScore;
}

std::string Computer::getPvList(Position board) {
    std::string pvList = "";
    std::set<U64> previousHashes;

    while (true) {
        U64 hash = board.board.getHash();
        // prevent cycles from transpositions
        if (previousHashes.find(hash) != previousHashes.end()) {
            return pvList;
        }
        previousHashes.insert(hash);

        auto res = searchTable.find(hash);
        if (res == searchTable.end() || res->second.pv.isNullMove()) {
            return pvList;
        }
        GenMove m = res->second.pv;
        if (pvList.length() > 0) {
            pvList += " ";
        }
        pvList += m.toLanMove().toString();
        board.movePseudoInPlace(m);
    }
}

void Computer::generateComputerInfo() {
    auto curr = std::chrono::high_resolution_clock::now();
    long micros = std::chrono::duration_cast<std::chrono::microseconds>(curr - task.lastTime).count();
    task.lastTime = curr;
    float seconds = micros / 1'000'000.0f;
    if (seconds == 0) {
        return;
    }

    ComputerInfo info;
    info.depth = task.iterativeDepth;
    info.nodes = task.prevTotalNodesSearched + task.currNodesSearched;
    info.nps = (long)((float)task.currNodesSearched / seconds);

    task.prevTotalNodesSearched += task.currNodesSearched;
    task.currNodesSearched = 0;

    auto hit = searchTable.find(task.rootPosition.board.getHash());
    if (hit == searchTable.end()) {
        printf("root not found\n");
        return;
    }
    SearchNode& node = hit->second;

    info.score = node.score;
    if (task.rootPosition.board.getSideToMove() == Side::Black) {
        info.score = -node.score;
    }

    info.pv = getPvList(task.rootPosition);

    std::lock_guard<std::mutex> guard(outputLock);
    infoBuffer.push_back(info);
}

class ScopedWorkingGuard {
   private:
    std::atomic<bool>& flag;

   public:
    explicit ScopedWorkingGuard(std::atomic<bool>& f) : flag(f) { flag = true; }
    ~ScopedWorkingGuard() { flag = false; }
};

/**
 * Expects that a task has been set on the computer.
 */
void Computer::launchSearch() {
    if (isWorking) {
        return;
    }

    // ensures is working will always be turned off on return
    ScopedWorkingGuard workingGuard(isWorking);

    searchTable.clear();

    accumulators.init(task.rootPosition.board);

    for (task.iterativeDepth = 1;; task.iterativeDepth++) {

        if (mustStopSearching()) {
            isWorking = false;
        }

        if (!isWorking) {
            break;
        }

        Score score = search(task.rootPosition, 0, -SCORE_CHECKMATE, SCORE_CHECKMATE);
        generateComputerInfo();

        bool isEndingPosition = std::abs(score) == SCORE_CHECKMATE;
        if (isEndingPosition) {
            // root position is terminal node
            break;
        }
    }

    auto currPv = searchTable.find(task.rootPosition.board.getHash());
    GenMove chosenMove = GenMove::NullMove();
    if (currPv != searchTable.end()) {
        chosenMove = currPv->second.pv;
    }

    std::lock_guard<std::mutex> guard(outputLock);
    bestMove.reset(new LanMove(chosenMove.toLanMove()));
}

long SearchParams::getLongField(const char* key) const {
    auto el = attributes.find(key);
    if (el != attributes.end()) {
        return el->second;
    }
    return -1;
}

bool SearchParams::getBoolField(const char* key) const {
    long asLong = getLongField(key);
    return asLong > 0;
}

