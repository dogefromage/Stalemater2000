#include "Computer.h"
#include <iostream>
#include <vector>
#include <array>

/*
bool Computer::Working = false;

std::unordered_map<U64, int> Computer::BestMoveTable;
std::unordered_map<U64, Position> Computer::PositionTable;
*/

void Computer::stop() {
    std::cerr << "Implement stop" << std::endl;
}

bool Computer::isWorking() {
    std::cerr << "Implement isWorking" << std::endl;
    return false;
}

void Computer::launchTest(ComputerTests testType, int depth) {
    std::cerr << "Implement launchTest" << std::endl;
}

void Computer::launchSearch(
    std::array<std::int64_t, (size_t)LongSearchParameters::SIZE> longParams,
    std::array<bool, (size_t)BoolSearchParameters::SIZE> boolParams,
    std::vector<LanMove> searchmoves
) {    
    std::cerr << "Implement launchSearch" << std::endl;
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