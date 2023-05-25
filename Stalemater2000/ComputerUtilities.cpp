#include "Computer.h"
#include <iostream>

/*
int Computer::randomMove(Board& board) {
    srand(time(NULL));

    MoveList moves;
    board.generateLegalMoves(moves);
    if (moves.size() == 0) {
        return -1;
    }
    int index = rand() % moves.size();
    return moves[index];
}

static unsigned long long* s_perftResults = NULL;
static int s_threadsFinished = 0;

void Computer::PerftAnalysis(Board board, int depth)
{
    Computer::Working = true;

    std::cout << "\nPerft Analysis Depth: " << depth << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // root perft multithreaded
    MoveList legalMoves;
    legalMoves.reserve(40);
    board.generateLegalMoves(legalMoves);
    int numberOfThreads = legalMoves.size();
    std::thread* threads = new std::thread[numberOfThreads];
    s_perftResults = new unsigned long long[numberOfThreads];
    s_threadsFinished = 0;

    std::cout << "Launching " << numberOfThreads << " threads... \n";

    for (int i = 0; i < numberOfThreads; i++)
    {
        Board nextBoard = board.movePseudo(legalMoves[i]);
        threads[i] = std::thread(Computer::perftThread, nextBoard, depth - 1, i);
    }

    while (s_threadsFinished < numberOfThreads && Computer::Working)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (!Computer::Working)
    {
        std::cout << "Perft search aborted...\n";
        delete[] s_perftResults;
        return;
    }

    std::uint64_t totalNodes = 0;
    for (int i = 0; i < numberOfThreads; i++)
    {
        unsigned long long nodes = s_perftResults[i];
        std::cout << Board::moveToString(legalMoves[i], true) << ", " << nodes << " Node(s)\n";
        totalNodes += nodes;
    }

    std::cout << "Total: " << totalNodes << " Node(s)\n";

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
    std::cout << "Total execution time: " << duration.count() << "s\n";

    delete[] s_perftResults;
    Computer::Working = false;
}

void Computer::perftThread(Board board, int depth, int index)
{
    std::uint64_t result = perft(board, depth);
    s_perftResults[index] = result;
    s_threadsFinished++;
}

std::uint64_t Computer::perft(Board& board, int depth)
{
    if (depth == 0) {
        return 1;
    }

    int moves = 0;
    MoveList pseudoMoves;
    pseudoMoves.reserve(40);
    board.generatePseudoMoves(pseudoMoves);
    Board nextBoard;
    for (const Move& m : pseudoMoves) {
        if (!Computer::Working) {
            break;
        }
        nextBoard = board.movePseudo(m);
        if (!nextBoard.isLegal()) {
            continue; // illegal move
        }
        moves += perft(nextBoard, depth - 1); // recurse
    }
    return moves;
}

static void ZobristRecurse(const Board& board, std::vector<int>& moveList, int depth) {
    if (depth == 0) {
        return;
    }
    MoveList pseudoMoves;
    pseudoMoves.reserve(40);
    board.generatePseudoMoves(pseudoMoves);
    Board nextBoard;
    for (const Move& m : pseudoMoves)
    {
        if (!Computer::Working)
            break;
        nextBoard = board.movePseudo(m);
        if (!nextBoard.isLegal())
            continue; // illegal move

        moveList.push_back(m);

        // CHECK ZOBRIST HASHS
        Board fenBoard = board; // copy
        fenBoard.generateZobristFromStart();

        if (fenBoard.zobrist != board.zobrist)
        {
            std::cout << "Wrong zobrist detected: \n";
            std::cout << std::hex << board.zobrist << ", " << fenBoard.zobrist << std::dec << "\nmoves:";
            for (int m : moveList)
            {
                std::cout << Board::moveToString(m, true) << " ";
            }
            std::cout << std::endl;
        }

        ZobristRecurse(nextBoard, moveList, depth - 1);
        moveList.pop_back();
    }
}

void Computer::ZobristTest(Board board, int depth)
{
    Computer::Working = true;
    std::cout << "Starting zobrist hash test... depth: " << depth << std::endl;
    std::vector<int> moveList;
    ZobristRecurse(board, moveList, depth);
    std::cout << "Zobrist test finished\n";
    Computer::Working = false;
}

*/