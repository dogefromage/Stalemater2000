#include "GameHistory.h"
#include <cassert>
#include <iostream>

GameHistory::GameHistory() {
    history.push_back(GamePosition::startPos());
}

GameHistory::GameHistory(GamePosition startNode) {
    history.push_back(startNode);
}

bool GameHistory::tryMoveLan(LanMove lanMove) {
    MoveList pseudoMoves;
    current().board.generatePseudoMoves(pseudoMoves);

    for (GenMove genMove : pseudoMoves) {
        if (genMove.matchesLanMove(lanMove)) {
            movePseudo(genMove);
            if (current().board.isLegal()) {
                return true;
            }
            moveBack();
        }
    }
    return false;
}

void GameHistory::movePseudo(GenMove move) {
    GamePosition pos(current()); // copy last pos
    HashBoard &board = pos.board;

    assert( (&current().board) != (&board) ); // idk if copy constructor also copies board

    bool isCapture = false;
    bool isPawnMove = false;

    MoveTypes moveType = move.movetype();
    if (moveType == MoveTypes::CastleWhiteKing) {
        board.movePieceOrCapture(BitBoards::KW, 4, 6); // set king to g1
        board.movePieceOrCapture(BitBoards::RW, 7, 5); // switch rook to f1
        board.forbidCastling(CastlingTypes::WhiteKing);
        board.forbidCastling(CastlingTypes::WhiteQueen);
    }
    else if (moveType == MoveTypes::CastleWhiteQueen) {
        board.movePieceOrCapture(BitBoards::KW, 4, 2); // set king to c1
        board.movePieceOrCapture(BitBoards::RW, 0, 3); // switch rook to d1
        board.forbidCastling(CastlingTypes::WhiteKing);
        board.forbidCastling(CastlingTypes::WhiteQueen);
    }
    else if (moveType == MoveTypes::CastleBlackKing) {
        board.movePieceOrCapture(BitBoards::KB, 60, 62);
        board.movePieceOrCapture(BitBoards::RB, 63, 61);
        board.forbidCastling(CastlingTypes::BlackKing);
        board.forbidCastling(CastlingTypes::BlackQueen);
    }
    else if (moveType == MoveTypes::CastleBlackQueen) {
        board.movePieceOrCapture(BitBoards::KB, 60, 58);
        board.movePieceOrCapture(BitBoards::RB, 56, 59);
        board.forbidCastling(CastlingTypes::BlackKing);
        board.forbidCastling(CastlingTypes::BlackQueen);
    }
    else {
        // non castle
        int fromSquare = move.from();
        int toSquare = move.to();
        U64 fromMask = 1ULL << fromSquare;
        U64 toMask = 1ULL << toSquare;

        // if castle piece is moved (or taken)
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_WHITE_KING)  board.forbidCastling(CastlingTypes::WhiteKing);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_WHITE_QUEEN) board.forbidCastling(CastlingTypes::WhiteQueen);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_BLACK_KING)  board.forbidCastling(CastlingTypes::BlackKing);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_BLACK_QUEEN) board.forbidCastling(CastlingTypes::BlackQueen);

        // exec normal move
        BitBoards bb = move.bitboard();
        isCapture = board.movePieceOrCapture(bb, fromSquare, toSquare);
        isPawnMove = bb == BitBoards::PW || bb == BitBoards::PB;

        bool isWhite = board.getSideToMove() == Side::White;

        // pawn specials
        if (moveType == MoveTypes::PawnDouble) {
            U64 nextEnpas = isWhite ? (fromMask << 8) : (fromMask >> 8);
            board.setEnpassantTarget(nextEnpas);
        }
        else if (moveType == MoveTypes::EnpasKing) {
            BitBoards otherPawns = isWhite ? BitBoards::PB : BitBoards::PW;
            board.removePiece(otherPawns, fromSquare + 1);
        }
        else if (moveType == MoveTypes::EnpasQueen) {
            BitBoards otherPawns = isWhite ? BitBoards::PB : BitBoards::PW;
            board.removePiece(otherPawns, fromSquare - 1);
        }
        else if (moveType == MoveTypes::Promote) {
            assert(move.promotion() != MovePromotions::None);

            int pawnBoard = (int)BitBoards::PW;
            int promBoard = (int)BitBoards::PW;

            switch (move.promotion()) {
            case MovePromotions::Q: promBoard = (int)BitBoards::PW; break;
            case MovePromotions::R: promBoard = (int)BitBoards::RW; break;
            case MovePromotions::N: promBoard = (int)BitBoards::NW; break;
            case MovePromotions::B: promBoard = (int)BitBoards::BW; break;
            }

            if (board.getSideToMove() == Side::Black) {
                pawnBoard += 6;
                promBoard += 6;
            }

            board.removePiece((BitBoards)pawnBoard, toSquare);
            board.placePiece((BitBoards)promBoard, toSquare);
        }
    }

    // reset enpassant target
    if (moveType != MoveTypes::PawnDouble) {
        board.setEnpassantTarget(0);
    }

    // update counter
    if (board.getSideToMove() == Side::Black) {
        pos.fullMovesCount++;
    }

    board.switchSide();

    // no-capture counter
    if (isCapture || isPawnMove) {
        pos.noCaptureOrPush = 0;
    } else {
        pos.noCaptureOrPush++;
    }

    history.push_back(pos);
}

void GameHistory::moveBack() {
    history.pop_back();
}

GamePosition& GameHistory::current() {
    assert(!history.empty());
    return history.back();
}

const GamePosition& GameHistory::current() const {
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