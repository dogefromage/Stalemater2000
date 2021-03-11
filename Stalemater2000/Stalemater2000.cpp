#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include "Board.h"
#include "Computer.h"
#include "Debug.h"
#include <bitset>

constexpr char const* ENGINENAME = "Stalemater2000";

using namespace std;
#include <mutex>
#include <atomic>

//https://stackoverflow.com/questions/16592357/non-blocking-stdgetline-exit-if-no-input
//This code works perfectly well on Windows 10 in Visual Studio 2015 c++ Win32 Console Debug and Release mode.
//If it doesn't work in your OS or environment, that's too bad; guess you'll have to fix it. :(
//You are free to use this code however you please, with one exception: no plagiarism!
//(You can include this in a much bigger project without giving any credit.)
class AsyncGetline
{
public:
    //AsyncGetline is a class that allows for asynchronous CLI getline-style input
    //(with 0% CPU usage!), which normal iostream usage does not easily allow.
    AsyncGetline()
    {
        input = "";
        sendOverNextLine = true;
        continueGettingInput = true;

        //Start a new detached thread to call getline over and over again and retrieve new input to be processed.
        thread([&]()
            {
                //Non-synchronized string of input for the getline calls.
                string synchronousInput;
                char nextCharacter;

                //Get the asynchronous input lines.
                do
                {
                    //Start with an empty line.
                    synchronousInput = "";

                    //Process input characters one at a time asynchronously, until a new line character is reached.
                    while (continueGettingInput)
                    {
                        //See if there are any input characters available (asynchronously).
                        while (cin.peek() == EOF)
                        {
                            //Ensure that the other thread is always yielded to when necessary. Don't sleep here;
                            //only yield, in order to ensure that processing will be as responsive as possible.
                            this_thread::yield();
                        }

                        //Get the next character that is known to be available.
                        nextCharacter = cin.get();

                        //Check for new line character.
                        if (nextCharacter == '\n')
                        {
                            break;
                        }

                        //Since this character is not a new line character, add it to the synchronousInput string.
                        synchronousInput += nextCharacter;
                    }

                    //Be ready to stop retrieving input at any moment.
                    if (!continueGettingInput)
                    {
                        break;
                    }

                    //Wait until the processing thread is ready to process the next line.
                    while (continueGettingInput && !sendOverNextLine)
                    {
                        //Ensure that the other thread is always yielded to when necessary. Don't sleep here;
                        //only yield, in order to ensure that the processing will be as responsive as possible.
                        this_thread::yield();
                    }

                    //Be ready to stop retrieving input at any moment.
                    if (!continueGettingInput)
                    {
                        break;
                    }

                    //Safely send the next line of input over for usage in the processing thread.
                    inputLock.lock();
                    input = synchronousInput;
                    inputLock.unlock();

                    //Signal that although this thread will read in the next line,
                    //it will not send it over until the processing thread is ready.
                    sendOverNextLine = false;
                } while (continueGettingInput && input != "exit");
            }).detach();
    }

    //Stop getting asynchronous CLI input.
    ~AsyncGetline()
    {
        //Stop the getline thread.
        continueGettingInput = false;
    }

    //Get the next line of input if there is any; if not, sleep for a millisecond and return an empty string.
    string GetLine()
    {
        //See if the next line of input, if any, is ready to be processed.
        if (sendOverNextLine)
        {
            //Don't consume the CPU while waiting for input; this_thread::yield()
            //would still consume a lot of CPU, so sleep must be used.
            this_thread::sleep_for(chrono::milliseconds(1));

            return "";
        }
        else
        {
            //Retrieve the next line of input from the getline thread and store it for return.
            inputLock.lock();
            string returnInput = input;
            inputLock.unlock();

            //Also, signal to the getline thread that it can continue
            //sending over the next line of input, if available.
            sendOverNextLine = true;

            return returnInput;
        }
    }

private:
    //Cross-thread-safe boolean to tell the getline thread to stop when AsyncGetline is deconstructed.
    atomic<bool> continueGettingInput;

    //Cross-thread-safe boolean to denote when the processing thread is ready for the next input line.
    //This exists to prevent any previous line(s) from being overwritten by new input lines without
    //using a queue by only processing further getline input when the processing thread is ready.
    atomic<bool> sendOverNextLine;

    //Mutex lock to ensure only one thread (processing vs. getline) is accessing the input string at a time.
    mutex inputLock;

    //string utilized safely by each thread due to the inputLock mutex.
    string input;
};

void tokenize(std::string const& str, const char delim, std::vector<std::string>& out)
{
    size_t start;
    size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
}

int searchKeyword(const vector<string>& command, string keyword)
{
    for (int i = 0; i < command.size(); i++)
    {
        if (command[i] == keyword)
        {
            return i;
        }
    }
    return -1;
}

int tryParseToInt(const vector<string>& command, int index)
{
    if (command.size() > index)
    {
        return stoi(command[index]);
    }
    else
    {
        return 0;
    }
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

bool e_debug = false;

int main()
{
    cout.setf(ios::unitbuf); // most important line in the whole engine
    // stolen from https://github.com/KierenP/Halogen/blob/master/Halogen/src/main.cpp you saved my life

    cout << ENGINENAME << " v1.0" << endl;

    InitZobrist();
    Board board = Board::Default();

    std::srand(time(0));

    string command;
    AsyncGetline ag;

    while (true)
    {
        command = ag.GetLine();
        if (command.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            std::string computerMsg = Computer::GetMessage();
            if (computerMsg.length() > 0)
            {
                cout << computerMsg;
            }

            continue;
        }

        vector<string> arguments;
        tokenize(command, ' ', arguments);

        if (arguments.size() > 0)
        {
            if (arguments[0] == "stop")
            {
                Computer::Working = false;
            }
            else if (!Computer::Working)
            {
                if (arguments[0] == "uci")
                {
                    cout << "id name " << ENGINENAME << "\n";
                    cout << "id author dogefromage\n";
                    cout << "uciok\n";
                }
                else if (arguments[0] == "isready")
                    cout << "readyok\n";
                else if (arguments[0] == "ucinewgame") { }
                else if (arguments[0] == "debug")
                {
                    if (arguments.size() > 1)
                    {
                        if (arguments[1] == "on")
                            e_debug = true;
                        else if (arguments[1] == "off")
                            e_debug = false;
                    }
                }
                else if (arguments[0] == "position")
                {
                    if (arguments.size() > 1)
                    {
                        int i = 2;
                        if (arguments[1] == "startpos")
                            board = Board::Default();
                        else if (arguments[1] == "fen")
                        {
                            string fen = "";
                            for (i = 2; i < arguments.size(); i++)
                            {
                                if (arguments[i] == "moves")
                                    break;
                                fen += arguments[i] + " ";
                            }
                            board = Board::FromFEN(fen);
                        }

                        if (i < arguments.size())
                        {
                            if (arguments[i] == "moves")
                            {
                                i++;

                                for (; i < arguments.size(); i++)
                                {
                                    string moveString = arguments[i];
                                    int from = Board::TextToIndex(moveString.substr(0, 2));
                                    int to = Board::TextToIndex(moveString.substr(2, 2));
                                    if (from < 0 || to < 0)
                                    {
                                        LOG("Error! couldn't interpret move: " + moveString);
                                        break;
                                    }
                                    int move = (to << 8) | from;
                                    if (moveString.size() > 4)
                                    {
                                        char promote = moveString[4];
                                        switch (promote)
                                        {
                                        case 'q':
                                            move |= MOVE_INFO_PROMOTE_QUEEN; break;
                                        case 'b':
                                            move |= MOVE_INFO_PROMOTE_BISHOP; break;
                                        case 'n':
                                            move |= MOVE_INFO_PROMOTE_HORSEY; break;
                                        case 'r':
                                            move |= MOVE_INFO_PROMOTE_ROOK; break;
                                        }
                                    }

                                    vector<MOVE> moves;
                                    board.GeneratePseudoMoves(moves);
                                    for (const MOVE& M : moves)
                                    {
                                        const int pm = M.second;

                                        if ((pm & 0xFFFF) == (move & 0xFFFF)) // match from, to and info (for promotion)
                                        {
                                            if (move & MOVE_TYPE_PROMOTE && !((pm & 0xFF000000) == (move & 0xFF000000)))
                                                continue; // info must only match if type is promote

                                            LOG("Move: " + Board::MoveToText(move, false));
                                            board = board.Move(pm); // use pseudo move so that the type is included
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                        LOG("Invalid command! expected: position <startpos | fen> <moves>");
                }
                else if (arguments[0] == "d")
                {
                    if (arguments.size() > 1)
                    {
                        if (arguments[1].length() == 1)
                        {
                            const char* boards = "PRNBQKprnbqk";
                            const char* bchar = std::strchr(boards, arguments[1][0]);
                            if (bchar != NULL)
                            {
                                int b = bchar - boards;
                                Board::PrintBitBoard(board.BitBoards[b]);
                            }
                        }
                        else
                        {
                            if (arguments[1] == "all")
                                board.PrintAllBitBoards();
                            else if (arguments[1] == "unsafe_w")
                                Board::PrintBitBoard(board.UnsafeForWhite);
                            else if (arguments[1] == "unsafe_b")
                                Board::PrintBitBoard(board.UnsafeForBlack);
                            else if (arguments[1] == "enpassant")
                                Board::PrintBitBoard(board.EnpassantTarget);
                            else if (arguments[1] == "empty")
                                Board::PrintBitBoard(board.Empty);
                            else if (arguments[1] == "occupied")
                                Board::PrintBitBoard(board.Occupied);
                            else if (arguments[1] == "not_w")
                                Board::PrintBitBoard(board.NotWhitePieces);
                            else if (arguments[1] == "not_b")
                                Board::PrintBitBoard(board.NotBlackPieces);
                            else if (arguments[1] == "pieces_w")
                                Board::PrintBitBoard(board.WhitePieces);
                            else if (arguments[1] == "pieces_b")
                                Board::PrintBitBoard(board.BlackPieces);
                            else if (arguments[1] == "eval")
                                cout << "Evaluation: " << Evaluation::evaluate(board) << endl;
                            else if (arguments[1] == "moves")
                            {
                                vector<MOVE> moves;
                                board.GenerateLegalMoves(moves);
                                for (const MOVE& m : moves)
                                    cout << Board::MoveToText(m.second, false) << endl;
                            }
                        }
                    }
                    else
                    {
                        board.Print();
                    }
                }
                else if (arguments[0] == "close" || arguments[0] == "exit" || arguments[0] == "quit")
                {
                    break;
                }
                else if (arguments[0] == "go")
                {
                    int perft = searchKeyword(arguments, "perft");
                    if (perft > 0)
                    {
                        try
                        {
                            int depth = stoi(arguments[perft + 1]);
                            if (depth > 0)
                            {
                                if (depth > 99) depth = 99;

                                thread worker(Computer::PerftAnalysis, board, depth);
                                worker.detach();
                            }
                        }
                        catch (exception e)
                        {
                            LOG("perft depth not understood");
                        }
                        continue;
                    }

                    int zobrist = searchKeyword(arguments, "zobrist");
                    if (zobrist > 0)
                    {
                        try
                        {
                            int depth = stoi(arguments[zobrist + 1]);
                            if (depth > 0)
                            {
                                if (depth > 99) depth = 99;

                                thread worker(Computer::ZobristTest, board, depth);
                                worker.detach();
                            }
                        }
                        catch (exception e)
                        {
                            LOG("zobrist depth not understood");
                        }
                        continue;
                    }

                    // NORMAL SEARCH
                    long long maxDepth = 10000000;
                    int depthKeyword = searchKeyword(arguments, "depth");
                    if (depthKeyword > 0)
                    {
                        long depth = tryParseToInt(arguments, depthKeyword + 1);
                        if (depth > 0)
                        {
                            maxDepth = depth;
                        }
                    }

                    std::thread worker(Computer::ChooseMove, board, maxDepth);
                    worker.detach();

                    int infinite = searchKeyword(arguments, "infinite");
                    if (infinite < 0 && depthKeyword < 0)
                    {
                        long long wTime = 10000000;
                        int wTimeKeyword = searchKeyword(arguments, "wtime");
                        if (wTimeKeyword > 0)
                        {
                            long long wt = tryParseToInt(arguments, wTimeKeyword + 1);
                            if (wt > 0)
                            {
                                wTime = wt;
                            }
                        }

                        long long bTime = 10000000;
                        int bTimeKeyword = searchKeyword(arguments, "btime");
                        if (bTimeKeyword > 0)
                        {
                            long long bt = tryParseToInt(arguments, bTimeKeyword + 1);
                            if (bt > 0)
                            {
                                bTime = bt;
                            }
                        }

                        long long remainingTime = bTime;
                        if (board.SideToMove == WHITE_TO_MOVE)
                            remainingTime = wTime;

                        std::thread timeManager(manageTime, remainingTime);
                        timeManager.detach();
                    }
                }
                else
                {
                    cout << "Unknown Command" << endl;
                }
            }
        }
    }
}