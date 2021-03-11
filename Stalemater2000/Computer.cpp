#include "Computer.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <tuple>
#include <queue>
#include <mutex>

#define U64 unsigned long long

bool Computer::Working = false;

std::unordered_map<U64, int> Computer::BestMoveTable;
std::unordered_map<U64, Position> Computer::PositionTable;

std::mutex messageLock;
std::queue<std::string> Computer::messages;

std::string Computer::GetMessage()
{
    messageLock.lock();

    std::string msg;

    if (messages.empty())
    {
        msg = "";
    }
    else
    {
        msg = messages.front();
        messages.pop();
    }

    messageLock.unlock();
    return msg;
}

void Computer::AddMessage(std::string msg)
{
    /*messageLock.lock();
    messages.push(msg);
    messageLock.unlock();*/
    std::cout << msg;
}

static unsigned long long nodesEvaluated = 0;
static unsigned long long transpositionsSkipped = 0;
static unsigned long long matesFound = 0;
static unsigned long long branchesPruned = 0;

void Computer::ChooseMove(Board board, int maxDepth)
{
    Working = true;
        
    nodesEvaluated = 0;
    transpositionsSkipped = 0;
    matesFound = 0;
    branchesPruned = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int depth = 1; depth <= maxDepth; depth++)
    {
        Score score = search(board, 0, depth, -0x6FFF, 0x6FFF);

        auto stop = std::chrono::high_resolution_clock::now();
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        if (!Working)
            break;

        std::string info = "";
        info +=  "info depth " + std::to_string(depth);
        
        bool seesMate = false;
        if (score > SCORE_CHECKMATE - 50)
        {
            info += " score mate " + std::to_string(SCORE_CHECKMATE - score);
            seesMate = true;
        }
        else if (score < -SCORE_CHECKMATE + 50)
        {
            info += " score mate " + std::to_string(-SCORE_CHECKMATE - score + 1);
            seesMate = true;
        }
        else
        {
            info += " score cp " + std::to_string(score);
        }

        info += " nodes " + std::to_string(nodesEvaluated);
        
        if (duration > 0)
        {
            info += " nps " + std::to_string(nodesEvaluated * 1000 / duration);
        }

        info += " pv ";
        Board pvBoard = board;

        for (int pvDepth = 0; pvDepth < depth; pvDepth++)
        {
            auto entry = BestMoveTable.find(pvBoard.Zobrist);
            if (entry == BestMoveTable.end())
                break;
            pvBoard = pvBoard.Move(entry->second);
            if (!pvBoard.GetBoardStatus())
                break;
            info += Board::MoveToText(entry->second, true) + " ";
        }

        info += "\n";
        AddMessage(info);
        
        LOG("NODES EVALUATED: " + std::to_string(nodesEvaluated));
        LOG("TRANSPOSITIONS SKIPPED: " + std::to_string(transpositionsSkipped));
        LOG("MATES FOUND: " + std::to_string(matesFound));
        LOG("BRANCHES PRUNED: " + std::to_string(branchesPruned));

        PositionTable.clear();

        if (seesMate)
        {
            break;
        }
    }

    auto entry = BestMoveTable.find(board.Zobrist);
    if (entry != BestMoveTable.end())
    {
        Board testBoard = board.Move(entry->second);
        if (testBoard.GetBoardStatus())
        {
            AddMessage("bestmove " + Board::MoveToText(entry->second, true) + "\n");
        }
    }

    PositionTable.clear();
    BestMoveTable.clear();
    
    Working = false;
}

Score Computer::search(const Board& board, int currDepth, int maxDepth, Score alpha, Score beta)
{
    if (currDepth == maxDepth)
    {
        //Score score = quiescence(board, alpha, beta);
        //return score - currDepth;

        Score score = Evaluation::evaluate(board); // evaluate, since no more captures
        if (board.SideToMove == BLACK_TO_MOVE)
            score = -score;
        nodesEvaluated++;
        return score - currDepth;
    }

    Score maxScore = -SCORE_CHECKMATE;
    int bestMove = 0;
    std::vector<MOVE> pseudoMoves;
    board.GeneratePseudoMoves(pseudoMoves);
    Board nextBoard;

    auto pvMoveEntry = BestMoveTable.find(board.Zobrist);
    int pvMove = 0;
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
            move = pseudoMoves[i - 1].second;
            if (move == pvMove)
                continue; // duplicate
        }

        nextBoard = board.Move(move);
        if (!nextBoard.GetBoardStatus())
            continue; // illegal

        Score score;

        // has entry?
        auto boardEntry = PositionTable.find(board.Zobrist);
        if (boardEntry != Computer::PositionTable.end())
        {
            // tranposition! this position was already searched during this search iteration
            transpositionsSkipped++;
            score = boardEntry->second.score - currDepth; // eval deep positions worse
        }
        else
        {
            // recurse
            score = -search(nextBoard, currDepth + 1, maxDepth, -beta, -alpha);
        }

        if (!Computer::Working) // break out if search aborted
        {
            break;
        }

        if (score > maxScore)
        {
            maxScore = score;
            bestMove = move;

            if (score > alpha)
            {
                alpha = score;
                if (alpha >= beta)
                {
                    // prune branch
                    branchesPruned++;
                    break;
                }
            }
        }
    }

    if (bestMove == 0) // no moves -> some sort of mate
    {
        matesFound++;
        // make sure no stalemate
        if (board.SideToMove == WHITE_TO_MOVE && !(board.Checks & CHECK_WHITE))
            maxScore = 0;
        else if (board.SideToMove == BLACK_TO_MOVE && !(board.Checks & CHECK_BLACK))
            maxScore = 0;
        else
        {
            // checkmate
            maxScore += currDepth - 1; // deeper mate is worse, also needed for info printout
        }
    }
    else
    {
        if (Computer::Working)
        {
            BestMoveTable[board.Zobrist] = bestMove;
        }
    }

    PositionTable[board.Zobrist] = { maxScore + (short)currDepth }; // add depth back on, bc. earlier it was removed

    return maxScore;
}

Score Computer::quiescence(const Board& board, Score alpha, Score beta)
{
    Score maxScore = -SCORE_CHECKMATE;
    int bestMove = 0;
    std::vector<MOVE> captures;
    board.GenerateCaptures(captures);
    Board nextBoard;

    for (const MOVE &m : captures)
    {
        int move = m.second;
        
        nextBoard = board.Move(move);
        if (!nextBoard.GetBoardStatus())
            continue; // illegal

        Score score;
        // has entry?
        auto boardEntry = PositionTable.find(board.Zobrist);
        if (boardEntry != Computer::PositionTable.end())
        {
            // tranposition! this position was already searched during this search iteration
            transpositionsSkipped++;
            score = boardEntry->second.score;
        }
        else
        {
            score = -quiescence(nextBoard, -beta, -alpha);
        }

        if (!Computer::Working) // break out if aborted
        {
            break;
        }

        if (score > maxScore)
        {
            maxScore = score;
            bestMove = move;

            if (score > alpha)
            {
                if (alpha >= beta)
                {
                    // prune branch
                    branchesPruned++;
                    break;
                }
            }
        }
    }

    if (bestMove == 0) // no move found
    {
        Score score = Evaluation::evaluate(board); // evaluate, since no more captures
        if (board.SideToMove == BLACK_TO_MOVE)
            score = -score;
        nodesEvaluated++;
        return score;
    }
    else
    {
        PositionTable[board.Zobrist] = maxScore;
        return maxScore;
    }
}

#undef U64