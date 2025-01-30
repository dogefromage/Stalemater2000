#include "computer.h"

#include <array>
#include <atomic>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <future>

#include "position.h"

std::mutex computerOutputLock;
std::vector<ComputerInfo> infoBuffer = {};
std::unique_ptr<LanMove> bestMove = NULL;

std::atomic<bool> isWorking = false;

struct SearchNode {
    Score score;
    short depth;
    GenMove pv;
};
std::unordered_map<U64, SearchNode> searchTable;

long currNodesSearched;
long prevTotalNodesSearched;
std::chrono::_V2::system_clock::time_point lastTime;
std::chrono::_V2::system_clock::time_point startTime;
int goalMicros = -1;
int iterativeDepth;
Position rootPosition;

long perft(Position& curr, int depth) {
    if (depth == 0) {
        return 1;
    }

    long count = 0;
    MoveList pseudoMoves;
    pseudoMoves.reserve(40);
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

void launchPerft(Position& root, int depth) {
    isWorking = true;

    long total = 0;

    MoveList pseudoMoves;
    pseudoMoves.reserve(40);
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

        std::cout << m.toLanMove().toString() << " -> " << moveScore << std::endl;
    }

    std::cout << "Total: " << total << std::endl;

    isWorking = false;
}

void launchZobrist(Position& root, int depth) {
    (void)root;
    (void)depth;
    std::cerr << "Implement zobrist" << std::endl;
}

void stopComputer() {
    if (!isWorking) {
        printf("computer is not working\n");
    }
    isWorking = false;
}

long* getSearchParamLongField(SearchParams* searchParams, std::string& name) {
    if (name == "wtime") return &searchParams->wtime;
    if (name == "btime") return &searchParams->btime;
    if (name == "winc") return &searchParams->winc;
    if (name == "binc") return &searchParams->binc;
    if (name == "movestogo") return &searchParams->movestogo;
    if (name == "depth") return &searchParams->depth;
    if (name == "nodes") return &searchParams->nodes;
    if (name == "mate") return &searchParams->mate;
    if (name == "movetime") return &searchParams->movetime;
    return nullptr;
}

bool* getSearchParamBoolField(SearchParams* searchParams, std::string& name) {
    if (name == "infinite") return &searchParams->infinite;
    if (name == "ponder") return &searchParams->ponder;
    return nullptr;
}

bool isComputerWorking() {
    return isWorking;
}

void launchTest(Position root, ComputerTests testType, int depth) {
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
static bool outOfTime() {
    if (goalMicros < 0) {
        return false;
    }
    auto curr = std::chrono::high_resolution_clock::now();
    long microsSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(curr - startTime).count();
    return microsSinceStart >= goalMicros;
}

Score search(Position curr, int remainingDepth, Score alpha, Score beta) {
    auto boardEntry = searchTable.find(curr.board.getHash());
    if (boardEntry != searchTable.end() &&
        boardEntry->second.depth >= remainingDepth) {
        // has already more knowledge over this node => skip
        return boardEntry->second.score;
    }

    currNodesSearched++;

    // printf("%ld nodes \n", currNodesSearched);

    if ((currNodesSearched % 100000 == 0) && outOfTime()) {
        isWorking = false;
        // printf("STOPPED WORKING\n");
    }

    if (remainingDepth <= 0 || !isWorking) {
        return evaluate(curr.board);
    }

    Score bestScore = -SCORE_CHECKMATE;
    GenMove bestMove = GenMove::NullMove();
    MoveList moves;
    curr.board.generatePseudoMoves(moves);

    for (GenMove& m : moves) {
        Position next(curr);
        next.movePseudoInPlace(m);
        if (!next.board.isLegal()) {
            continue;
        }

        Score score = -search(next, remainingDepth - 1, -beta, -alpha);

        // if (!isWorking) {
        //     printf("curr move %s\n", m.toString().c_str());
        // }

        if (score >= beta) {
            // prune branch
            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            if (score > alpha) {
                alpha = score;
            }
        }
    }

    bool stalemate = false;  // add counting remaining pieces and threefold repetition

    if (bestMove.isNullMove()) {
        // no moves
        if (curr.board.getSideToMove() == Side::White) {
            if (!(curr.board.hasCheck(CheckFlags::WhiteInCheck))) {
                // white has no moves but is not in check
                stalemate = true;
            }
        } else if (!(curr.board.hasCheck(CheckFlags::BlackInCheck))) {
            stalemate = true;
        }
    }
    if (stalemate) {
        bestScore = 0;
    }

    if (isWorking) {
        searchTable[curr.board.getHash()] = {.score = bestScore, .depth = (short)remainingDepth, .pv=bestMove};
    }

    return bestScore;
}

std::string getPvList(Position board) {
    std::string pvList = "";
    while (true) {
        auto res = searchTable.find(board.board.getHash());
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

void generateComputerInfo() {
    auto curr = std::chrono::high_resolution_clock::now();
    long micros = std::chrono::duration_cast<std::chrono::microseconds>(curr - lastTime).count();
    lastTime = curr;
    double seconds = micros / 1'000'000.0;
    if (seconds == 0) {
        return;
    }
    
    ComputerInfo info;
    info.depth = iterativeDepth;
    info.nodes = prevTotalNodesSearched + currNodesSearched;
    info.nps = (long)((double)currNodesSearched / seconds);

    prevTotalNodesSearched += currNodesSearched;
    currNodesSearched = 0;

    auto hit = searchTable.find(rootPosition.board.getHash());
    if (hit == searchTable.end()) {
        printf("root not found\n");
        return;
    }
    SearchNode& node = hit->second;
    info.score = node.score;

    info.pv = getPvList(rootPosition);

    std::lock_guard<std::mutex> guard(computerOutputLock);

    infoBuffer.push_back(info);
}

void launchSearch(
    Position root,
    SearchParams params,
    std::vector<LanMove> searchmoves) {

    if (isWorking) {
        return; // instantly stop
    }
    isWorking = true;

    (void)searchmoves;

    startTime = lastTime = std::chrono::high_resolution_clock::now();
    goalMicros = -1;
    // int wbTimeMillis = root.board.getSideToMove() == Side::White ? params.movetime : params.btime;

    int moveTime = params.movetime;
    if (moveTime < 0) {
        moveTime = 3000;
    }

    if (moveTime >= 0) {
        goalMicros = 1000 * moveTime;
        // printf("goal micros = %ld\n", goalMicros);
    } 

    rootPosition = root;
    currNodesSearched = prevTotalNodesSearched = 0;

    int maxDepth = params.depth >= 0 ? params.depth : 10000;

    for (iterativeDepth = 1; iterativeDepth <= maxDepth; iterativeDepth++) {

        search(root, iterativeDepth, -SCORE_CHECKMATE, SCORE_CHECKMATE);

        if (isWorking) {
            generateComputerInfo();
        } else {
            break;
        }
    }

    auto currPv = searchTable.find(root.board.getHash());
    if (currPv == searchTable.end() ||
        currPv->second.pv.isNullMove()) {
        printf("search failed - no move found\n");
    } else {
        std::lock_guard<std::mutex> guard(computerOutputLock);
        bestMove.reset(new LanMove(currPv->second.pv.toLanMove()));
    }

    isWorking = false;
}

/*
void Computer::AddMessage(std::string msg) {
    std::cout << msg;
}

static unsigned long long nodesEvaluated = 0;
static unsigned long long transpositionsSkipped = 0;
static unsigned long long matesFound = 0;
static unsigned long long branchesPruned = 0;

void Computer::ChooseMove(const Board &board, int maxDepth)
{
    Working = true;

    nodesEvaluated = 0;
    transpositionsSkipped = 0;
    matesFound = 0;
    branchesPruned = 0;

    auto start = std::chrono::high_resolution_clock::now();

    int bestMove = 0;

    for (int depth = 1; depth <= maxDepth; depth++)
    {
        Score score = search(board, depth, -SCORE_CHECKMATE - 1, SCORE_CHECKMATE + 1);

        auto stop = std::chrono::high_resolution_clock::now();
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        if (!Working)
        {
            // LOG("computer stopped");
            break;
        }

        std::string message = "";
        message +=  "info depth " + std::to_string(depth);

        bool seesMate = false;
        if (score > SCORE_CHECKMATE - 50)
        {
            int mateDepth = 1 + SCORE_CHECKMATE - score;
            mateDepth = ceil(mateDepth / 2);

            message += " score mate " + std::to_string(mateDepth);
            seesMate = true;
        }
        else if (score < -SCORE_CHECKMATE + 50)
        {
            int mateDepth = 3 - SCORE_CHECKMATE - score;
            mateDepth = floor(mateDepth / 2);

            message += " score mate " + std::to_string(mateDepth);
            seesMate = true;
        }
        else
        {
            message += " score cp " + std::to_string(score);
        }

        message += " nodes " + std::to_string(nodesEvaluated);

        if (duration > 0)
        {
            message += " nps " + std::to_string(nodesEvaluated * 1000 / duration);
        }

        message += " pv ";
        Board pvBoard = board;

        for (int pvDepth = 0; pvDepth < depth; pvDepth++)
        {
            auto entry = BestMoveTable.find(pvBoard.zobrist);
            if (entry == BestMoveTable.end())
            {
                // message += DEBUGMESSAGE("no entry ");
                break;
            }
            pvBoard = pvBoard.movePseudo(entry->second);

            if (pvDepth == 0)
            {
                bestMove = entry->second;
            }

            if (entry->second != 0)
            {
                if (pvBoard.isLegal())
                {
                    message += Board::moveToString(entry->second, true) + " ";
                }
                else
                {
                    // message += DEBUGMESSAGE("move is illegal ");
                }
            }
            else
            {
                // message += DEBUGMESSAGE("move is zero ");
            }
        }

        message += "\n";
        AddMessage(message);

        // LOG("NODES EVALUATED: " + std::to_string(nodesEvaluated));
        // LOG("TRANSPOSITIONS SKIPPED: " + std::to_string(transpositionsSkipped));
        // LOG("MATES FOUND: " + std::to_string(matesFound));
        // LOG("BRANCHES PRUNED: " + std::to_string(branchesPruned));

        if (seesMate)
        {
            break;
        }
    }

    if (bestMove)
    {
        AddMessage("bestmove " + Board::moveToString(bestMove, true) + "\n");
    }
    else
    {
        // LOG("resign");
    }

    PositionTable.clear();
    BestMoveTable.clear();

    Working = false;
}

Score Computer::search(const Board& board, int remainingDepth, Score alpha, Score beta)
{
    if (remainingDepth == 0)
    {
        Score score = quiescence(board, alpha, beta);
        return score;
    }

    Score bestScore = -SCORE_CHECKMATE;

    int bestMove = 0;
    MoveList pseudoMoves;
    board.generatePseudoMoves(pseudoMoves);
    Board nextBoard;

    int pvMove = 0;
    auto pvMoveEntry = BestMoveTable.find(board.zobrist);
    if (pvMoveEntry != BestMoveTable.end())
    {
        pvMove = pvMoveEntry->second;
    }

    for (int i = 0; i <= pseudoMoves.size(); i++)
    {
        int move;
        if (i == 0)
        {
            if (!pvMove)
                continue; // no pv
            move = pvMove;
        }
        else
        {
            move = pseudoMoves[i - 1];
            if (move == pvMove)
                continue; // duplicate
        }

        nextBoard = board.movePseudo(move);
        if (!nextBoard.isLegal())
            continue; // illegal

        Score score;

        bool validTransposition = false;

        // has entry?
        auto boardEntry = PositionTable.find(board.zobrist);
        if (boardEntry != Computer::PositionTable.end())
        {
            if (boardEntry->second.depth >= remainingDepth)
            {
                // has already more knowledge over this node => skip
                transpositionsSkipped++;
                score = boardEntry->second.score;
                validTransposition = true;
            }
        }

        if (!validTransposition)
        {
            // recurse
            score = -search(nextBoard, remainingDepth - 1, -beta, -alpha);

            // favor shallower positions
            if (score > 0)
            {
                score--;
            }
            else if (score < 0)
            {
                score++;
            }
        }

        if (score >= beta)
        {
            // prune branch
            branchesPruned++;
            return score;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;

            if (score > alpha)
            {
                alpha = score;
            }
        }

        if (!Computer::Working) // break out if search aborted
        {
            break;
        }
    }

    bool stalemate = board.isStalemate(); // evalates remaining pieces or threefold repetition

    if (pseudoMoves.size() == 0)
    {
        matesFound++;

        if (board.sideToMove == WHITE_TO_MOVE)
        {
            if ( !(board.checks & CHECK_WHITE) ) // white has no moves but is not in check
            {
                stalemate = true;
            }
        }
        else
        {
            if (!(board.checks & CHECK_BLACK)) // same here
            {
                stalemate = true;
            }
        }
    }
    else
    {
        if (Computer::Working)
        {
            BestMoveTable[board.zobrist] = bestMove;
        }
    }

    if (stalemate)
    {
        bestScore = 0;
    }

    PositionTable[board.zobrist] = { bestScore, (short)remainingDepth };

    return bestScore;
}

Score Computer::quiescence(const Board& board, Score alpha, Score beta)
{
    Score standPat = Evaluation::evaluate(board); // evaluate, since no more captures
    nodesEvaluated++;

    if (board.sideToMove == BLACK_TO_MOVE)
        standPat = -standPat;

    if (standPat >= beta)
    {
        // prune
        branchesPruned++;
        return standPat;
    }
    if (standPat > alpha)
    {
        alpha = standPat;
    }

    MoveList captures;
    board.generatePseudoCaptures(captures);
    Board nextBoard;

    for (const Move move : captures)
    {
        nextBoard = board.movePseudo(move);
        if (!nextBoard.isLegal())
            continue; // illegal

        // here, directly evaluating is faster than looking up transpositions from the table

        Score score = -quiescence(nextBoard, -beta, -alpha);

        if (!Computer::Working) // break out if aborted
        {
            break;
        }

        if (score >= beta)
        {
            // prune branch
            branchesPruned++;
            return beta;
        }

        if (score > alpha)
        {
            alpha = score;
        }
    }

    return alpha;
}




void manageTime(long long remainingTime)
{
    LOG("remaining: " + std::to_string(remainingTime));

    auto start = std::chrono::high_resolution_clock::now();

    long long waitTime = remainingTime / 10LL;
    if (waitTime > 10000LL)
    {
        waitTime = 8000LL + (std::rand() % 4000LL);
    }

    while (true)
    {
        auto stop = std::chrono::high_resolution_clock::now();
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        if (!Computer::Working)
        {
            break;
        }

        if (duration > waitTime) // % of all the time
        {
            LOG("TIME MANAGER HAS HALTED SEARCH BECAUSE TIME USED UP");
            LOG("Remaining time: " + to_string(remainingTime) + ", search duration: " + to_string(duration));
            Computer::Working = false;

            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

*/