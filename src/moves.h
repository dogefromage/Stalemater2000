#pragma once
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "labels.h"

struct LanMove {
    int from, to;
    MovePromotions promotion;

    LanMove(int from, int to, MovePromotions promotion) 
        : from(from), to(to), promotion(promotion) {}

    std::string toString() const;
    static std::string squareIndexToString(int index);
    static std::optional<LanMove> parseLanMove(const std::string& moveString);
    static std::optional<int> parseSquareIndex(const std::string& squareName);
};

struct GenMove {
    int from, to;
    MovePromotions promotion;
    BitBoards bb;
    MoveTypes type;

    GenMove(int from, int to, MovePromotions promotion, BitBoards bb, MoveTypes type) 
        : from(from), to(to), promotion(promotion), bb(bb), type(type) {}

    // matches promotion, to, from
    bool matchesLanMove(const LanMove& lanm) const {
        return from == lanm.from && to == lanm.to && promotion == lanm.promotion;
    }

    std::string toString() const;
    LanMove toLanMove() const;

    static GenMove NullMove() {
        return GenMove(0, 0, MovePromotions::None, BitBoards::PW, MoveTypes::Normal);
    }

    bool isNullMove() {
        return from == 0 && to == 0;
    }
};

typedef std::vector<GenMove> MoveList;