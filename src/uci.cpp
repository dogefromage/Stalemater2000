#include "uci.h"

#include <array>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <thread>

#include "history.h"
#include "log.h"
#include "moves.h"
#include "position.h"

std::set<const char*, StringComparator> allLongParams = {
    "wtime", "btime", "winc", "binc", "movestogo", "depth", "nodes", "mate", "movetime"};
std::set<const char*, StringComparator> allBoolParams = {
    "infinite", "ponder"};

const std::string ENGINE_NAME = "Stalemater2000";

Computer computer;

std::optional<std::string> nextKeyword(std::list<std::string>& keywords, const std::string& expectedToken) {
    if (keywords.empty()) {
        std::cout << "Invalid command, found end of line, expected [" << expectedToken << "]" << std::endl;
        return std::nullopt;
    }
    std::string firstToken = keywords.front();
    keywords.pop_front();
    return {firstToken};
}

std::optional<int> nextInteger(std::list<std::string>& keywords, const std::string& expectedToken) {
    std::optional<std::string> keyword = nextKeyword(keywords, expectedToken);
    if (keyword.has_value()) {
        try {
            int val = std::stoi(keyword.value());
            return {val};
        } catch (const std::exception& _) {
            std::cout << "Expected integer for [" << expectedToken << "]" << std::endl;
        }
    }
    return std::nullopt;
}

UCI::UCI() {
    std::cout << ENGINE_NAME << std::endl;
    hist = History();
}

void tokenize(std::string const& str, const char delim, std::list<std::string>& out) {
    size_t start;
    size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
}

void UCI::writeTokenizedCommand(std::string rawLine) {
    LOG("[IN] %s\n", rawLine.c_str());

    std::list<std::string> tokenizedLine;
    tokenize(rawLine, ' ', tokenizedLine);

    if (tokenizedLine.empty()) {
        return;
    }
    std::string firstToken = tokenizedLine.front();
    tokenizedLine.pop_front();

    if (firstToken == "stop")
        handleStop(tokenizedLine);
    else if (firstToken == "uci")
        handleUci(tokenizedLine);
    else if (firstToken == "isready")
        handleIsReady(tokenizedLine);
    else if (firstToken == "go")
        handleGo(tokenizedLine);
    else if (firstToken == "position")
        handlePosition(tokenizedLine);
    else if (firstToken == "ucinewgame")
        handleUciNewGame(tokenizedLine);
    else if (firstToken == "d")
        handleDisplay(tokenizedLine);
    else if (firstToken == "quit")
        handleQuit(tokenizedLine);
    else if (firstToken == "movelist")
        handleMovelist(tokenizedLine);
    else {
        printf("ERROR unknown command entered \"%s\"\n", firstToken.c_str());
    }
}

void UCI::consumeOutput() {
    std::lock_guard<std::mutex> guard(computer.outputLock);

    for (const ComputerInfo& info : computer.infoBuffer) {
        char score_string[100];

        if (info.score < -MAX_EVAL || info.score > MAX_EVAL) {
            // convert to mate number
            int sign = info.score > 0 ? 1 : -1;
            int mateRel = (SCORE_CHECKMATE - std::abs(info.score) + 1) / 2;
            int mateNumber = mateRel * sign;

            sprintf(score_string, "score mate %d", mateNumber);
        } else {
            sprintf(score_string, "score cp %ld", info.score);
        }

        printf("info depth %ld %s nodes %ld nps %ld pv %s\n",
               info.depth, score_string, info.nodes, info.nps, info.pv.c_str());
    }
    computer.infoBuffer.clear();

    if (computer.bestMove != NULL) {
        printf("bestmove %s\n", computer.bestMove->toString().c_str());
        computer.bestMove.reset();
    }
}

void UCI::handleUci(std::list<std::string>& params) {
    (void)params;
    std::cout << "id name " << ENGINE_NAME << std::endl;
    std::cout << "id author dogefromage" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::handleIsReady(std::list<std::string>& params) {
    (void)params;
    std::cout << "readyok" << std::endl;
}

void UCI::handleUciNewGame(std::list<std::string>& params) {
    (void)params;

    if (computer.isWorking) {
        computer.stopWorking();
    }
}

void UCI::handleGo(std::list<std::string>& params) {
    if (computer.isWorking) {
        printf("Cannot start search, computer is working");
        return;
    }

    if (!params.empty()) {
        std::string firstKeyword = params.front();
        bool isPerft = firstKeyword == "perft";
        bool isZobrist = firstKeyword == "zobrist";
        if (isPerft || isZobrist) {
            params.pop_front();
            std::optional<int> depth = nextInteger(params, "depth");
            if (!depth.has_value()) return;

            ComputerTests testType;
            if (isPerft) testType = ComputerTests::Perft;
            if (isZobrist) testType = ComputerTests::Zobrist;

            std::thread testThread(&Computer::launchTest, &computer, hist.current(), testType, depth.value());
            testThread.detach();
            return;
        }
    }

    // NORMAL SEARCH
    // https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
    ComputerSearchTask newTask(hist.current());
    newTask.params.attributes = {
        {"wtime", -1},
        {"btime", -1},
        {"winc", -1},
        {"binc", -1},
        {"movestogo", -1},
        {"depth", -1},
        {"nodes", -1},
        {"mate", -1},
        {"movetime", -1},
        {"infinite", 0},
        {"ponder", 0},
    };

    while (!params.empty()) {
        std::string param = nextKeyword(params, "search parameter").value();
        const char* paramStr = param.c_str();

        if (allLongParams.find(paramStr) != allLongParams.end()) {
            std::optional<int> optInt = nextInteger(params, param + " value");
            if (!optInt.has_value()) {
                continue;
            }
            newTask.params.attributes[paramStr] = optInt.value();
            continue;
        }

        if (allBoolParams.find(paramStr) != allBoolParams.end()) {
            newTask.params.attributes[paramStr] = 1;
            continue;
        }

        // searchmoves
        if (param == "searchmoves") {
            while (!params.empty()) {
                std::string searchMove = nextKeyword(params, "searchmove move").value();
                std::optional<LanMove> parsedMove = LanMove::parseLanMove(searchMove);
                if (parsedMove.has_value()) {
                    newTask.params.searchmoves.push_back(parsedMove.value());
                } else {
                    printf("invalid move \"%s\"\n", searchMove.c_str());
                }
            }
            continue;
        }

        printf("ERROR invalid param \"%s\"\n", param.c_str());
    }

    computer.task = newTask;
    std::thread searchThread(&Computer::launchSearch, &computer);
    searchThread.detach();
}

void UCI::handlePosition(std::list<std::string>& params) {
    std::optional<std::string> positionTypeOpt = nextKeyword(params, "startpos|fen");
    if (!positionTypeOpt.has_value()) return;
    std::string positionType = positionTypeOpt.value();

    if (positionType == "startpos") {
        hist = History(Position::startPos());
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
        hist = History(Position::fromFen(fenTokens));
    }

    if (params.empty()) {
        return;
    }
    std::string movesKeyword = nextKeyword(params, "moves").value();
    if (movesKeyword != "moves") {
        printf("ERROR expected [moves]\n");
        return;
    }

    while (!params.empty()) {
        std::string moveKeyword = nextKeyword(params, "param").value();
        std::optional<LanMove> move = LanMove::parseLanMove(moveKeyword);
        if (!move.has_value()) {
            printf("ERROR invalid move \"%s\"\n", moveKeyword.c_str());
            return;
        }
        bool success = hist.tryMoveLan(move.value());
        if (!success) {
            printf("ERROR could not make move in current position \"%s\"\n", move.value().toString().c_str());
            return;
        }
    }
}

void UCI::handleDisplay(std::list<std::string>& params) {
    bool moves = false;

    if (!params.empty()) {
        std::string arg = params.front();
        params.pop_front();
        if (arg == "moves") {
            moves = true;
        } else {
            printf("ERROR unknown print arg \"%s\"\n", arg.c_str());
        }
    }

    hist.current().print(moves);
}

void UCI::handleStop(std::list<std::string>& params) {
    (void)params;
    computer.stopWorking();
}

void UCI::handleQuit(std::list<std::string>& params) {
    (void)params;
    exit(EXIT_SUCCESS);
}

void UCI::handleMovelist(std::list<std::string>& params) {
    (void)params;
    MoveList moves;
    hist.current().generateLegalMoves(moves);

    std::cout << "Legal moves:" << std::endl;

    for (GenMove& m : moves) {
        std::cout << m.toString() << std::endl;
    }
}

constexpr auto TERMINAL_RESET = "\033[0m";
constexpr auto TERMINAL_RED = "\033[31m";
