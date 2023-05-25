#include "moves.h"
#include <iostream>

std::string LanMove::toString() const {
    std::string msg = squareIndexToString(from()) + squareIndexToString(to());

    switch (promotion()) {
    case MovePromotions::Q: msg += "q"; break;
    case MovePromotions::R: msg += "r"; break;
    case MovePromotions::N: msg += "n"; break;
    case MovePromotions::B: msg += "b"; break;
    }

    return msg;
}

std::string LanMove::squareIndexToString(int index) {
    return "abcdefgh"[index % 8] + std::to_string(index / 8 + 1);
}

std::optional<LanMove> LanMove::parseLanMove(const std::string& moveString) {
    std::optional<int> s = parseSquareIndex(moveString.substr(0, 2));
    std::optional<int> t = parseSquareIndex(moveString.substr(2, 2));
    if (!s.has_value() || !t.has_value()) {
        return std::nullopt;
    }

    MovePromotions promotion = MovePromotions::None;

    if (moveString.size() > 4) {
        char promote = tolower(moveString[4]);
        switch (promote) {
        case 'q': promotion = MovePromotions::Q; break;
        case 'b': promotion = MovePromotions::B; break;
        case 'n': promotion = MovePromotions::N; break;
        case 'r': promotion = MovePromotions::R; break;
        default:
            std::cerr << "Unknown promotion type: " << promote << std::endl;
        }
    }
    return { LanMove(s.value(), t.value(), promotion) };
}

std::optional<int> LanMove::parseSquareIndex(const std::string& squareName) {
    if (squareName.length() != 2) {
        std::cerr << "Invalid square: " << squareName << std::endl;
        return std::nullopt;
    }
    // file starting at 'a', rank starting at '1' * 8
    int index = tolower(squareName[0]) - 'a'
        + 8 * (squareName[1] - '1');
    if (index < 0 || index > 63) {
        std::cerr << "Invalid square: " << squareName << std::endl;
        return std::nullopt;
    }
    return { index };
}

std::string GenMove::toString() const {
    std::string msg = LanMove::squareIndexToString(from()) 
                    + LanMove::squareIndexToString(to());

    switch (promotion()) {
    case MovePromotions::Q: msg += "q"; break;
    case MovePromotions::R: msg += "r"; break;
    case MovePromotions::N: msg += "n"; break;
    case MovePromotions::B: msg += "b"; break;
    }
    
    msg += " (bb=" + std::to_string((int)bitboard())
        + ", type=" + std::to_string((int)movetype()) + ")";

    return msg;
}
