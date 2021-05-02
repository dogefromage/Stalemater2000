#include "Computer.h"

#define U64 unsigned long long

int Computer::randomMove(Board& board)
{
    srand(time(NULL));

    std::vector<MOVE> moves;
    board.GenerateLegalMoves(moves);
    if (moves.size() == 0)
        return 0;
    int index = rand() % moves.size();
    return moves[index].second;
}

static unsigned long long* s_perftResults = NULL;
static int s_threadsFinished = 0;

void Computer::PerftAnalysis(Board board, int depth)
{
    Computer::Working = true;

    std::cout << "\nPerft Analysis Depth: " << depth << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // root perft multithreaded
    std::vector<MOVE> legalMoves;
    legalMoves.reserve(40);
    board.GenerateLegalMoves(legalMoves);
    int numberOfThreads = legalMoves.size();
    std::thread* threads = new std::thread[numberOfThreads];
    s_perftResults = new unsigned long long[numberOfThreads];
    s_threadsFinished = 0;

    std::cout << "Launching " << numberOfThreads << " threads... \n";

    for (int i = 0; i < numberOfThreads; i++)
    {
        Board nextBoard = board.Move(legalMoves[i].second);
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

    unsigned long long totalNodes = 0;
    for (int i = 0; i < numberOfThreads; i++)
    {
        unsigned long long nodes = s_perftResults[i];
        std::cout << Board::MoveToText(legalMoves[i].second, true) << ", " << nodes << " Node(s)\n";
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
    unsigned long long result = perft(board, depth);
    s_perftResults[index] = result;
    s_threadsFinished++;
}

unsigned long long Computer::perft(Board& board, int depth)
{
    if (depth == 0)
        return 1;

    int moves = 0;
    std::vector<MOVE> pseudoMoves;
    pseudoMoves.reserve(40);
    board.GeneratePseudoMoves(pseudoMoves);
    Board nextBoard;
    for (const MOVE& m : pseudoMoves)
    {
        if (!Computer::Working)
            break;

        nextBoard = board.Move(m.second);
        if (!nextBoard.IsLegal())
            continue; // illegal move

        moves += perft(nextBoard, depth - 1); // recurse
    }

    return moves;
}

static void ZobristRecurse(const Board& board, std::vector<int>& moveList, int depth)
{
    if (depth == 0)
        return;

    std::vector<MOVE> pseudoMoves;
    pseudoMoves.reserve(40);
    board.GeneratePseudoMoves(pseudoMoves);
    Board nextBoard;
    for (const MOVE& m : pseudoMoves)
    {
        if (!Computer::Working)
            break;

        nextBoard = board.Move(m.second);
        if (!nextBoard.IsLegal())
            continue; // illegal move

        moveList.push_back(m.second);

        // CHECK ZOBRIST HASHS
        Board fenBoard = board; // copy
        fenBoard.GenerateZobrist();

        if (fenBoard.Zobrist != board.Zobrist)
        {
            std::cout << "Wrong zobrist detected: \n";
            std::cout << std::hex << board.Zobrist << ", " << fenBoard.Zobrist << std::dec << "\nmoves:";
            for (int m : moveList)
            {
                std::cout << Board::MoveToText(m, true) << " ";
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

#undef U64