#pragma once
#include <string>
#include <vector>

#include "moves.h"
#include "position.h"

class History {
   public:
    History();
    History(Position rootPosition);

    bool tryMoveLan(LanMove move);
    void moveBack();

    const Position& current() const;
    Position& current();

   private:
    std::vector<Position> history;
};