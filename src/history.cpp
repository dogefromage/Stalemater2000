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

/*
bool Board::isStalemate() const {
    // 50 move rule
    if (noCaptureOrPush >= 50) {
        return true;
    }

    // threefold rep.
    int numberOfRepetitions = 1;

    const Board* board = this->lastBoard;
    for (int i = 0; i < 100; i++)
    {
        if (board == nullptr)
        {
            break;
        }

        if (zobrist == board->zobrist)
        {
            numberOfRepetitions++;
        }

        if (numberOfRepetitions >= 3)
        {
            return true;
        }

        if (board->noCaptureOrPush == 0)
        {
            break;
        }

        board = board->lastBoard;

        if (board == board->lastBoard || board == nullptr)
        {
            break;
        }

        if (i == 99) {
            std::cerr << "IsStalemate() has run for 100 iterations (probably loop in lastBoard pointers)" << std::endl;
        }
    }

    //material
    bool sufficientMaterial = false;
    if (boards[PW] | boards[RW] | boards[QW] | boards[PB] | boards[RB] | boards[QB])
    {
        // at least one winning piece
        sufficientMaterial = true;
    }
    else if (countBits(boards[BW]) > 1 || countBits(boards[BB]) > 1)
    {
        // at least two bishops
        sufficientMaterial = true;
    }
    else if ((boards[BW] != 0 && boards[NW] != 0) || (boards[BB] != 0 && boards[NB] != 0))
    {
        // one or more bishops and one or more knights
        sufficientMaterial = true;
    }

    return !sufficientMaterial;
}
*/