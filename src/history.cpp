#include "history.h"

#include <cassert>
#include <iostream>

History::History() {
    history.push_back(Position::startPos());
}

History::History(Position startNode) {
    history.push_back(startNode);
}

bool History::tryMoveLan(LanMove lanMove) {
    MoveList pseudoMoves;
    current().board.generatePseudoMoves(pseudoMoves);

    GenMove correctMove = GenMove::NullMove();

    for (GenMove genMove : pseudoMoves) {
        if (genMove.matchesLanMove(lanMove)) {
            correctMove = genMove;
            break;
        }
    }

    if (correctMove.isNullMove()) {
        return false; // move was not in list
    }

    Position next(current());
    next.movePseudoInPlace(correctMove);
    if (!next.board.isLegal()) {
        return false; // move is illegal
    }

    history.push_back(next);
    
    return true;
}

void History::moveBack() {
    history.pop_back();
}

Position& History::current() {
    assert(!history.empty());
    return history.back();
}

const Position& History::current() const {
    assert(!history.empty());
    return history.back();
}
