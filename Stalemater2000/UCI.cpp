#include "UCI.h"
#include "moves.h"
#include <iostream>
#include <optional>
#include <array>
#include "GameHistory.h"
#include "GamePosition.h"

const std::string ENGINENAME = "Stalemater2000";

std::optional<std::string> nextKeyword(std::list<std::string>& keywords, const std::string& expectedToken) {
    if (keywords.empty()) {
        std::cout << "Invalid command, found end of line, expected [" << expectedToken << "]" << std::endl;
        return std::nullopt;
    }
    std::string firstToken = keywords.front();
    keywords.pop_front();
    return { firstToken };
}

std::optional<int> nextInteger(std::list<std::string>& keywords, const std::string& expectedToken) {
    std::optional<std::string> keyword = nextKeyword(keywords, expectedToken);
    if (keyword.has_value()) {
        try {
            int val = std::stoi(keyword.value());
            return { val };
        }
        catch (const std::exception& _) {
            std::cout << "Expected integer for [" << expectedToken << "]" << std::endl;
        }
    }
    return std::nullopt;
}

UCI::UCI() {
	std::cout << ENGINENAME << std::endl;
	game = GameHistory();
    computer = Computer();
}

void UCI::writeTokenizedCommand(std::list<std::string>& tokenizedLine) {
    if (tokenizedLine.empty()) {
        return;
    }
    std::string firstToken = tokenizedLine.front();
    tokenizedLine.pop_front();

    if      (firstToken == "stop")       handleStop(tokenizedLine);
    else if (firstToken == "uci")        handleUci(tokenizedLine);
    else if (firstToken == "isready")    handleIsReady(tokenizedLine);
    else if (firstToken == "go")         handleGo(tokenizedLine);
    else if (firstToken == "position")   handlePosition(tokenizedLine);
    else if (firstToken == "ucinewgame") handleUciNewGame(tokenizedLine);
    else if (firstToken == "d")          handleDisplay(tokenizedLine);
    else if (firstToken == "quit")       handleQuit(tokenizedLine);
    else {
        std::cout << "Unknown command entered \"" << firstToken << "\"" << std::endl;
    }
}

void UCI::handleUci(std::list<std::string>& params)
{
    std::cout << "id name " << ENGINENAME << std::endl;
    std::cout << "id author dogefromage" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::handleIsReady(std::list<std::string>& params) {
    std::cout << "readyok" << std::endl;
}

void UCI::handleUciNewGame(std::list<std::string>& params)
{
    // ?
}

void UCI::handleGo(std::list<std::string>& params)
{
    if (!params.empty()) {
        std::string firstKeyword = params.front();
        bool isPerft = firstKeyword == "perft";
        bool isZobrist = firstKeyword == "zobrist";
        if (isPerft || isZobrist) {
            params.pop_front();
            std::optional<int> depth = nextInteger(params, "depth");
            if (!depth.has_value()) return;

            ComputerTests testType;
            if (isPerft)   testType = ComputerTests::Perft;
            if (isZobrist) testType = ComputerTests::Zobrist;

            computer.launchTest(testType, depth.value());
            return;
        }
    }

    // NORMAL SEARCH
    // https://www.wbec-ridderkerk.nl/html/UCIProtocol.html

    auto longParams = std::array<std::int64_t, (size_t)LongSearchParameters::SIZE>();
    longParams.fill(-1);
    auto boolParams = std::array<bool, (size_t)BoolSearchParameters::SIZE>();
    boolParams.fill(false);

    std::vector<LanMove> searchMoves;

    while (!params.empty()) {
        std::string param = nextKeyword(params, "search parameter").value();

        // longs
        LongSearchParameters lParam = LongSearchParameters::SIZE;
        if (param == "wtime") lParam = LongSearchParameters::wtime;
        if (param == "btime") lParam = LongSearchParameters::btime;
        if (param == "winc") lParam = LongSearchParameters::winc;
        if (param == "binc") lParam = LongSearchParameters::binc;
        if (param == "movestogo") lParam = LongSearchParameters::movestogo;
        if (param == "depth") lParam = LongSearchParameters::depth;
        if (param == "nodes") lParam = LongSearchParameters::nodes;
        if (param == "mate") lParam = LongSearchParameters::mate;
        if (param == "movetime") lParam = LongSearchParameters::movetime;

        if (lParam != LongSearchParameters::SIZE) {
            std::optional<int> optInt = nextInteger(params, param + " value");
            if (!optInt.has_value()) {
                continue;
            }
            longParams[(size_t)lParam] = optInt.value();
            continue;
        }

        // bools
        BoolSearchParameters bParam = BoolSearchParameters::SIZE;
        if (param == "infinite") bParam = BoolSearchParameters::infinite;
        if (param == "ponder") bParam = BoolSearchParameters::ponder;

        if (lParam != LongSearchParameters::SIZE) {
            boolParams[(size_t)lParam] = true;
            continue;
        }

        // searchmoves
        if (param == "searchmoves") {
            while (!params.empty()) {
                std::string searchMove = nextKeyword(params, "searchmove move").value();
                std::optional<LanMove> parsedMove = LanMove::parseLanMove(searchMove);
                if (parsedMove.has_value()) {
                    searchMoves.push_back(parsedMove.value());
                }
            }
            continue;
        }

        printError("Unknown search param: " + param + "\n");
    }

    computer.launchSearch(longParams, boolParams, searchMoves);
}

void UCI::handlePosition(std::list<std::string>& params) {
    GameHistory nextGame;

    std::optional<std::string> positionTypeOpt = nextKeyword(params, "startpos|fen");
    if (!positionTypeOpt.has_value()) return;
    std::string positionType = positionTypeOpt.value();

    if (positionType == "startpos") {
        nextGame = GameHistory(GamePosition::startPos());
    } else if (positionType == "fen") {
        std::vector<std::string> fenTokens;
        while (!params.empty()) {
            std::string nextPart = params.front();
            if (nextPart == "moves") {
                break;
            }
            fenTokens.push_back(nextPart);
            params.pop_front();
        }
        nextGame = GameHistory(GamePosition::fromFen(fenTokens));
    }
    
    if (params.empty()) {
        return;
    }
    std::string movesKeyword = nextKeyword(params, "moves").value();
    if (movesKeyword != "moves") {
        printError("expected [moves]\n");
        return;
    }

    while (!params.empty()) {
        std::string moveKeyword = nextKeyword(params, "param").value();
        std::optional<LanMove> move = LanMove::parseLanMove(moveKeyword);
        if (!move.has_value()) {
            return;
        }
        bool success = nextGame.tryMoveLan(move.value());
        if (!success) {
            printError("Could not execute move: " + move.value().toString() + "\n");
            return;
        }
    }

    game = nextGame;
}

void UCI::handleDisplay(std::list<std::string>& params) {
    bool moves = false;

    if (!params.empty()) {
        std::string arg = params.front();
        params.pop_front();
        if (arg == "moves") {
            moves = true;
        } else {
            printError("Unknown print arg: " + arg + "\n");
        }
    }

    game.current().print(moves);
}

void UCI::handleStop(std::list<std::string>& params) {
    computer.stop();
}

void UCI::handleQuit(std::list<std::string>& params) {
    exit(EXIT_SUCCESS);
}

constexpr auto TERMINAL_RESET = "\033[0m";
constexpr auto TERMINAL_RED = "\033[31m";

void UCI::printError(std::string msg) const {
    std::cout << TERMINAL_RED << msg << TERMINAL_RESET;
}

