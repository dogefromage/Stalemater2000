#include "computer.h"

#include <vector>

#include "position.h"
#include <set>

long Computer::perft(Position& curr, int depth) {
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

void Computer::launchPerft(Position& root, int depth) {
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

    if (task->params.getBoolField("infinite")) {
        return false; // never stop here
    }

    // depth <x>
    long depthParam = task->params.getLongField("depth");
    if (depthParam > 0 && task->iterativeDepth > depthParam) {
        return true;
    }

    // nodes <x>, mate <x>, 
    // TODO

    auto curr = std::chrono::high_resolution_clock::now();
    long totalMillis = std::chrono::duration_cast<std::chrono::milliseconds>(curr - task->startTime).count();
    
    // movetime <ms>
    long movetimeParam = task->params.getLongField("movetime");
    if (movetimeParam > 0 && totalMillis >= movetimeParam) {
        return true;
    }

    long fullMovesCount = task->rootPosition.fullMovesCount;

    float wtime = (float)task->params.getLongField("wtime");
    float btime = (float)task->params.getLongField("btime");
    float winc = (float)task->params.getLongField("winc");
    float binc = (float)task->params.getLongField("binc");
    long movestogo = task->params.getLongField("movestogo");

    bool isWhite = task->rootPosition.board.getSideToMove() == Side::White;
    float remainingTime = isWhite ? wtime : btime;
    float increment = isWhite ? winc : binc;

    if (remainingTime > 0) {
        // do time management
        if (increment < 0) {
            increment = 0;
        }
        if (movestogo <= 0) {
            // sudden death
            movestogo = std::max(15L, 50L - fullMovesCount);
        }

        float availableTime = remainingTime + increment * movestogo;
        float moveLimitMillis = availableTime / movestogo;

        // limit time in opening
        float minOpeningFactor = 0.33;
        int openingFullMoves = 8;
        float openingFactor = std::max(minOpeningFactor, std::min(fullMovesCount / (float)openingFullMoves, 1.0f));

        moveLimitMillis *= openingFactor;

        // factor in some delay
        moveLimitMillis = 0.95 * (moveLimitMillis - 500);

        if (totalMillis > moveLimitMillis) {
            return true;
        }
    }

    return false;
}

Score Computer::search(Position curr, int remainingDepth, Score alpha, Score beta) {

    auto boardEntry = searchTable.find(curr.board.getHash());
    if (boardEntry != searchTable.end() &&
        boardEntry->second.depth >= remainingDepth) {
        // has already more knowledge over this node => skip
        return boardEntry->second.score;
    }

    task->currNodesSearched++;

    // only rarely check if out of time
    if (task->currNodesSearched % 100000 == 0) {
        if (mustStopSearching()) {
            isWorking = false;
        }
    }

    if (remainingDepth <= 0 || !isWorking) {
        return evaluate_relative(curr.board);
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

        if (bestMove.isNullMove()) {
            bestMove = m;
        }

        Score score = -search(next, remainingDepth - 1, -beta, -alpha);
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
        searchTable[curr.board.getHash()] = {.score = bestScore, .depth = (short)remainingDepth, .pv = bestMove};
    }

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
    long micros = std::chrono::duration_cast<std::chrono::microseconds>(curr - task->lastTime).count();
    task->lastTime = curr;
    float seconds = micros / 1'000'000.0f;
    if (seconds == 0) {
        return;
    }

    ComputerInfo info;
    info.depth = task->iterativeDepth;
    info.nodes = task->prevTotalNodesSearched + task->currNodesSearched;
    info.nps = (long)((float)task->currNodesSearched / seconds);

    task->prevTotalNodesSearched += task->currNodesSearched;
    task->currNodesSearched = 0;

    auto hit = searchTable.find(task->rootPosition.board.getHash());
    if (hit == searchTable.end()) {
        printf("root not found\n");
        return;
    }
    SearchNode& node = hit->second;
    info.score = node.score;

    info.pv = getPvList(task->rootPosition);

    std::lock_guard<std::mutex> guard(outputLock);
    infoBuffer.push_back(info);
}

void Computer::launchSearch(ComputerSearchTask* newTask) {
    if (isWorking) {
        free(newTask);
        return;  // instantly stop
    }
    task = newTask;
    isWorking = true;

    // this would not be necessary but I think my search algorithm is not sound
    searchTable.clear();

    for (task->iterativeDepth = 1; ; task->iterativeDepth++) {
        if (mustStopSearching()) {
            isWorking = false;
        }

        if (!isWorking) {
            break;
        }

        search(task->rootPosition, task->iterativeDepth, -SCORE_CHECKMATE, SCORE_CHECKMATE);
        generateComputerInfo();
    }

    auto currPv = searchTable.find(task->rootPosition.board.getHash());
    if (currPv == searchTable.end() ||
        currPv->second.pv.isNullMove()) {
        printf("search failed - no move found\n");
    } else {
        std::lock_guard<std::mutex> guard(outputLock);
        bestMove.reset(new LanMove(currPv->second.pv.toLanMove()));
    }

    isWorking = false;
    free(task);
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
