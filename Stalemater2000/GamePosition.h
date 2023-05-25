#pragma once
#include "HashBoard.h"

class GamePosition {
public:
    HashBoard board;
    short fullMovesCount = 1;
    short noCaptureOrPush = 0;

    std::string toFen() const;
    void print();
    void print(bool moves);

    static GamePosition startPos();
    static GamePosition fromFen(const std::vector<std::string>& arguments);
};

