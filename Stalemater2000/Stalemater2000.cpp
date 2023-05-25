#include <iostream>
#include <string>
#include <list>
#include "UCI.h"
#include "Zobrist.h"

void tokenize(std::string const& str, const char delim, std::list<std::string>& out) {
    size_t start;
    size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
}

int main() {
    std::cout.setf(std::ios::unitbuf); // most important line in the whole engine
    // stolen from https://github.com/KierenP/Halogen/blob/master/Halogen/src/main.cpp you saved my life

    InitZobrist();

    UCI uci;

    while (true) {
        std::string inputLine;
        std::getline(std::cin, inputLine);

        std::list<std::string> tokenizedLine;
        tokenize(inputLine, ' ', tokenizedLine);
     
        uci.writeTokenizedCommand(tokenizedLine);
    }
}
