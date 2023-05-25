#pragma once
#include <string>
#include <vector>
#include "moves.h"
#include "GamePosition.h"

class GameHistory {
public:
    GameHistory();
    GameHistory(GamePosition rootPosition);

    bool tryMoveLan(LanMove move);
    void movePseudo(GenMove move);
    void moveBack();

    const GamePosition& current() const;
    GamePosition& current();

private:
    std::vector<GamePosition> history;
};