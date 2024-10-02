#pragma once
#include <string>
#include <vector>

#include "board.h"
#include "hash.h"

class Position {
   public:
    Board board;
    short fullMovesCount = 1;
    short noCaptureOrPush = 0;

    std::string toFen() const;
    void print();
    void print(bool moves);

    static Position startPos();
    static Position fromFen(const std::vector<std::string>& arguments);

    void movePseudoInPlace(GenMove move);
};
