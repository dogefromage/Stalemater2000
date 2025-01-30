#include "uci.h"

#include <array>
#include <cstring>
#include <iostream>
#include <optional>
#include <thread>

#include "history.h"
#include "moves.h"
#include "position.h"
#include "log.h"

const std::string ENGINE_NAME = "Stalemater2000";

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
    // std::unique_lock<std::mutex> guard(computerOutputLock, std::try_to_lock);
    std::lock_guard<std::mutex> guard(computerOutputLock);

    for (const ComputerInfo& info : infoBuffer) {
        printf("info depth %ld score cp %ld nodes %ld nps %ld pv %s\n", 
            info.depth, info.score, info.nodes, info.nps, info.pv.c_str());
    }
    infoBuffer.clear();

    if (bestMove != NULL) {
        printf("bestmove %s\n", bestMove->toString().c_str());
        bestMove.reset();
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
    
    if (isComputerWorking()) {
        stopComputer();
    }
}

void UCI::handleGo(std::list<std::string>& params) {
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

            std::thread testThread(launchTest, hist.current(), testType, depth.value());
            testThread.detach();
            return;
        }
    }

    // NORMAL SEARCH
    // https://www.wbec-ridderkerk.nl/html/UCIProtocol.html

    SearchParams searchParams;
    searchParams.wtime = -1;
    searchParams.btime = -1;
    searchParams.winc = -1;
    searchParams.binc = -1;
    searchParams.movestogo = -1;
    searchParams.depth = -1;
    searchParams.nodes = -1;
    searchParams.mate = -1;
    searchParams.movetime = -1;
    searchParams.infinite = false;
    searchParams.ponder = false;

    std::vector<LanMove> searchMoves;

    while (!params.empty()) {
        std::string param = nextKeyword(params, "search parameter").value();

        bool* boolField;
        if ((boolField = getSearchParamBoolField(&searchParams, param))) {
            *boolField = true;
            continue;
        }

        long* longField;
        if ((longField = getSearchParamLongField(&searchParams, param))) {
            std::optional<int> optInt = nextInteger(params, param + " value");
            if (!optInt.has_value()) {
                continue;
            }
            *longField = optInt.value();
            continue;
        }

        // searchmoves
        if (param == "searchmoves") {
            while (!params.empty()) {
                std::string searchMove = nextKeyword(params, "searchmove move").value();
                std::optional<LanMove> parsedMove = LanMove::parseLanMove(searchMove);
                if (parsedMove.has_value()) {
                    searchMoves.push_back(parsedMove.value());
                } else {
                    printf("invalid move \"%s\"\n", searchMove.c_str());
                }
            }
            continue;
        }

        printf("ERROR invalid param \"%s\"\n", param.c_str());
    }

    std::thread searchThread(launchSearch, hist.current(), searchParams, searchMoves);
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
    stopComputer();
}

void UCI::handleQuit(std::list<std::string>& params) {
    (void)params;
    exit(EXIT_SUCCESS);
}

void UCI::handleMovelist(std::list<std::string>& params) {
    (void)params;
    MoveList all, legal;
    hist.current().board.generatePseudoMoves(all);

    for (GenMove& m : all) {
        Position next(hist.current());
        next.movePseudoInPlace(m);
        if (next.board.isLegal()) {
            legal.push_back(m);
        }
    }

    std::cout << "Legal moves:" << std::endl;

    for (GenMove& m : legal) {
        std::cout << m.toString() << std::endl;
    }
}

constexpr auto TERMINAL_RESET = "\033[0m";
constexpr auto TERMINAL_RED = "\033[31m";
