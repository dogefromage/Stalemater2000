#pragma once
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <array>

#include "labels.h"

struct LanMove {
    int from, to;
    MovePromotions promotion;

    LanMove()
        : from(0), to(0), promotion(MovePromotions::None) {}

    LanMove(int from, int to, MovePromotions promotion)
        : from(from), to(to), promotion(promotion) {}

    static LanMove NullMove() {
        return LanMove();
    }

    bool isNullMove() const {
        return from == 0 && to == 0;
    }

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
    CaptureType capture;
    int score;

    GenMove()
        : from(0), to(0), promotion(MovePromotions::None), bb(BitBoards::PW), type(MoveTypes::Normal), capture(CaptureType::NonCapture), score(0) {}

    GenMove(int from, int to, MovePromotions promotion, BitBoards bb, MoveTypes type, CaptureType capture)
        : from(from), to(to), promotion(promotion), bb(bb), type(type), capture(capture), score(0) {}

    // matches promotion, to, from
    bool matchesLanMove(const LanMove& lanm) const {
        return from == lanm.from && to == lanm.to && promotion == lanm.promotion;
    }

    std::string toString() const;
    LanMove toLanMove() const;

    static GenMove NullMove() {
        return GenMove();
    }

    bool isNullMove() {
        return from == 0 && to == 0;
    }
};

struct MoveList {
    int size = 0;
    std::array<GenMove, MAXIMUM_POSSIBLE_MOVES> list;

    void add(GenMove m) {
        assert(size < MAXIMUM_POSSIBLE_MOVES);
        list[size++] = m;
    }

    auto begin() { return list.begin(); }
    auto end() { return list.begin() + size; }
};
