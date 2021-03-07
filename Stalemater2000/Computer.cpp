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

static short currentSearch = 0;
static unsigned long long nodesEvaluated = 0;
static unsigned long long transpositionsSkipped = 0;
static unsigned long long matesFound = 0;
static unsigned long long branchesPruned = 0;

std::unordered_map<U64, Position> Computer::PositionTable;

extern std::string messageOut = "";

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
    messageLock.lock();
    messages.push(msg);
    messageLock.unlock();
}

void Computer::Init()
{
    PositionTable.clear();
    currentSearch = 0;
}

//bool compareBoardsMaximizing(const std::tuple<Score, Board, int>& left,
//                             const std::tuple<Score, Board, int>& right)
//{
//    const Score &scoreLeft = std::get<0>(left);
//    const Score &scoreRight = std::get<0>(right);
//    if (scoreLeft == SCORE_NONE)
//        return false;
//
//    if (scoreRight == SCORE_NONE)
//        return true;
//
//    return (scoreLeft > scoreRight);
//}
//
//bool compareBoardsMinimizing(const std::tuple<Score, Board, int>& left,
//    const std::tuple<Score, Board, int>& right)
//{
//    const Score& scoreLeft = std::get<0>(left);
//    const Score& scoreRight = std::get<0>(right);
//    if (scoreLeft == SCORE_NONE)
//        return false;
//
//    if (scoreRight == SCORE_NONE)
//        return true;
//
//    return (scoreLeft < scoreRight);
//}
//
//Score Computer::search(const Board& board, int depth, Score alpha, Score beta)
//{
//    if (depth == 0)
//    {
//        Score score = Evaluation::evaluate(board);
//        PositionTable[board.Zobrist] = Position(score, 0, currentSearch);
//        nodesEvaluated++;
//        return score;
//    }
//
//    Score maxScore = -0xFFF; // not INT_MIN because negating INT_MIN will overflow :(
//    int bestMove = 0;
//
//    // moves
//    std::vector<int> pseudoMoves;
//    pseudoMoves.reserve(40);
//    board.GeneratePseudoMoves(pseudoMoves);
//
//    // score, board, move that led to it
//    std::vector<std::tuple<Score, Board, int>> boardTuples;
//    boardTuples.reserve(pseudoMoves.size());
//
//    // calculate all boards
//    for (int move : pseudoMoves)
//    {
//        const Board &newBoard = board.Move(move);
//        if (!board.GetBoardStatus())
//            continue; // illegal
//
//        auto boardEntry = Computer::PositionTable.find(newBoard.Zobrist);
//        Score score = SCORE_NONE;
//        if (boardEntry != Computer::PositionTable.end())
//        {
//            score = boardEntry->second.score;
//            if (boardEntry->second.searchID == currentSearch)
//            {
//                // tranposition! this position was already searched during this iteration
//                transpositionsSkipped++;
//                //continue; 
//            }
//        }
//
//        boardTuples.push_back({score, newBoard, move});
//    }
//
//    if (board.SideToMove == WHITE_TO_MOVE)
//        std::sort(boardTuples.begin(), boardTuples.end(), compareBoardsMaximizing);
//    else
//        std::sort(boardTuples.begin(), boardTuples.end(), compareBoardsMinimizing);
//
//    for (auto boardTuple : boardTuples)
//    {
//        Board& nextBoard = std::get<1>(boardTuple);
//
//        Score score = -search(nextBoard, depth - 1, -beta, -alpha);
//
//        if (!Computer::Working) // break out if aborted
//        {
//            if (score > maxScore)
//            {
//                maxScore = score;
//                bestMove = std::get<2>(boardTuple);
//            }
//
//            break;
//        }
//
//        if (score > maxScore)
//        {
//            maxScore = score;
//            bestMove = std::get<2>(boardTuple);
//
//            alpha = std::max(alpha, score);
//            if (alpha >= beta)
//            {
//                branchesPruned++;
//                break; // prune branch
//            }
//        }
//    }
//
//    if (bestMove == 0) // no moves -> some sort of mate
//    {
//        matesFound++;
//        // make sure no stalemate
//        if (board.SideToMove == WHITE_TO_MOVE && !(board.Checks & CHECK_WHITE))
//            maxScore = 0;
//        if (board.SideToMove == BLACK_TO_MOVE && !(board.Checks & CHECK_BLACK))
//            maxScore = 0;
//    }
//
//    PositionTable[board.Zobrist] = Position(maxScore, bestMove, currentSearch);
//
//    return maxScore;
//}

void Computer::ChooseMove(Board board, int maxDepth)
{
    Working = true;
        
    int bestMove = 0;
    
    nodesEvaluated = 0;
    transpositionsSkipped = 0;
    matesFound = 0;
    branchesPruned = 0;

    std::cout << "\n";
    
    for (int depth = 1; depth <= maxDepth; depth++)
    //for (int depth = maxDepth; depth <= maxDepth; depth++)
    {
        search(board, depth, -0x6FFF, 0x6FFF);

        if (!Working)
            break;

        std::string info = "";

        // PRINT INFO
        auto posEntry = PositionTable.find(board.Zobrist);
        if (posEntry != PositionTable.end())
        {
            const Position& pos = posEntry->second;
    
            info +=  "info depth " + std::to_string(depth);
            Score score = pos.score;
            if (board.SideToMove == BLACK_TO_MOVE)
                score = -score;
    
            info += " score cp " + std::to_string(pos.score);

            // LIST PV
            if (pos.bestMove != 0)
            {
                bestMove = pos.bestMove;
                int m = bestMove;
                info += " pv ";
                Board pvBoard = board;
                while (m != 0)
                {
                    info += Board::MoveToText(m, true) + " ";
                    pvBoard = pvBoard.Move(m);
                    if (!pvBoard.GetBoardStatus())
                        break;
                    auto nextEntry = PositionTable.find(pvBoard.Zobrist);
                    if (nextEntry == PositionTable.end())
                        break;
                     
                    m = nextEntry->second.bestMove;
                }
            }

            LOG("NODES EVALUATED: " + std::to_string(nodesEvaluated));
            LOG("TRANSPOSITIONS SKIPPED: " + std::to_string(transpositionsSkipped));
            LOG("MATES FOUND: " + std::to_string(matesFound));
            LOG("BRANCHES PRUNED: " + std::to_string(branchesPruned));

            info += "\n";
            
            AddMessage(info);
        }
    
        currentSearch++;
    }
    
    if (bestMove != 0)
    {
        AddMessage("bestmove " + Board::MoveToText(bestMove, true) + "\n");
    }
    
    Init();
    
    Working = false;
}

Score Computer::search(const Board& board, int depth, Score alpha, Score beta)
{
    if (depth == 0)
    {
        Score score = Evaluation::evaluate(board);
        if (board.SideToMove == BLACK_TO_MOVE)
            score = -score;
        PositionTable[board.Zobrist] = Position(score, 0, currentSearch);
        nodesEvaluated++;
        return score;
    }

    Score maxScore = -0x6FFF;
    int bestMove = 0;
    std::vector<MOVE> pseudoMoves;
    board.GeneratePseudoMoves(pseudoMoves);
    Board nextBoard;
    for (MOVE move : pseudoMoves)
    {
        nextBoard = board.Move(move.second);
        if (!nextBoard.GetBoardStatus())
            continue; // illegal

        Score score = -search(nextBoard, depth - 1, -beta, -alpha);

        if (!Computer::Working) // break out if aborted
        {
            break;
        }

        if (score > maxScore)
        {
            maxScore = score;
            bestMove = move.second;

            alpha = std::max(alpha, score);
            if (alpha >= beta)
            {
                // prune branch
                branchesPruned++;
                break; 
            }
        }
    }

    if (bestMove == 0) // no moves -> some sort of mate
    {
        matesFound++;
        // make sure no stalemate
        if (board.SideToMove == WHITE_TO_MOVE && !(board.Checks & CHECK_WHITE))
            maxScore = 0;
        if (board.SideToMove == BLACK_TO_MOVE && !(board.Checks & CHECK_BLACK))
            maxScore = 0;
    }

    if (Computer::Working)
    {
        PositionTable[board.Zobrist] = Position(maxScore, bestMove, currentSearch);
    }
    
    return maxScore;
}

#undef U64