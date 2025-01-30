#include "position.h"

#include <string.h>
#include <iostream>

#include "bitmath.h"
#include <cassert>

Position Position::startPos() {
    std::vector<std::string> fen = {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1"};
    return fromFen(fen);
}

Position Position::fromFen(const std::vector<std::string>& arguments) {
    Position node = Position();
    Board& board = node.board;

    size_t argumentIndex = 0;

    // only so that i can break out of block (this is actually genius)
    while (true) {
        if (argumentIndex >= arguments.size()) break;
        const std::string& piecePositions = arguments[argumentIndex];
        argumentIndex++;

        // PIECE POSITIONS
        int i = 0, j = 7;  // decrease j so that LSB is A0 and not A7 like in FEN
        char srchString[] = "PRNBQKprnbqk12345678/";
        int strIndex = 0;
        while (true) {
            const char* fenChar = strchr(srchString, piecePositions[strIndex++]);
            if (fenChar == NULL) {
                break;  // end of piece description
            }

            int charIndex = (int)(fenChar - srchString);
            if (charIndex < 12)  // letter
            {
                if (i < 8)  // safety if too many chars
                {
                    assert(0 <= charIndex && charIndex < 12);
                    board.placePiece((BitBoards)charIndex, 8 * j + i);
                    i++;
                }
            } else if (charIndex < 20)  // number
            {
                i += charIndex - 11;
            } else  // '/'
            {
                i = 0;
                j--;
                if (j < 0) {
                    break;
                }
            }
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& sideToMove = arguments[argumentIndex];
        argumentIndex++;

        // SideToMove
        if (sideToMove == "w") {
            // default is white
        } else if (sideToMove == "b") {
            board.switchSide();
        } else {
            break;
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& castling = arguments[argumentIndex];
        argumentIndex++;

        // Castling
        int castlingRights = 0;
        for (size_t i = 0; i < castling.length(); i++) {
            switch (castling[i]) {
                case 'K':
                    castlingRights |= 1;
                    break;
                case 'Q':
                    castlingRights |= 2;
                    break;
                case 'k':
                    castlingRights |= 4;
                    break;
                case 'q':
                    castlingRights |= 8;
                    break;
            }
        }
        for (int i = 0; i < 4; i++) {
            if ((castlingRights & (1 << i)) == 0) {
                board.forbidCastling((CastlingTypes)i);
            }
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& enpas = arguments[argumentIndex];
        argumentIndex++;

        // enpassant square
        if (enpas != "-") {
            std::optional<int> enpasSquare = LanMove::parseSquareIndex(enpas);
            if (enpasSquare.has_value()) {
                board.setEnpassantTarget(1ULL << enpasSquare.value());
            }
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& halfMoveClock = arguments[argumentIndex];
        argumentIndex++;

        // halfmoveclocl
        int hmc = std::stoi(halfMoveClock);
        if (hmc > 0) {
            node.noCaptureOrPush = hmc;
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& fullMoves = arguments[argumentIndex];
        argumentIndex++;

        // full moves
        int fm = std::stoi(fullMoves);
        if (fm > 0) {
            node.fullMovesCount = fm;
        }

        break;
    }

    return node;
}

void Position::movePseudoInPlace(GenMove move) {

    assert(!move.isNullMove());

    bool isCapture = false;
    bool isPawnMove = false;

    MoveTypes moveType = move.type;
    if (moveType == MoveTypes::CastleWhiteKing) {
        board.movePieceOrCapture(BitBoards::KW, 4, 6);  // set king to g1
        board.movePieceOrCapture(BitBoards::RW, 7, 5);  // switch rook to f1
        board.forbidCastling(CastlingTypes::WhiteKing);
        board.forbidCastling(CastlingTypes::WhiteQueen);
    } else if (moveType == MoveTypes::CastleWhiteQueen) {
        board.movePieceOrCapture(BitBoards::KW, 4, 2);  // set king to c1
        board.movePieceOrCapture(BitBoards::RW, 0, 3);  // switch rook to d1
        board.forbidCastling(CastlingTypes::WhiteKing);
        board.forbidCastling(CastlingTypes::WhiteQueen);
    } else if (moveType == MoveTypes::CastleBlackKing) {
        board.movePieceOrCapture(BitBoards::KB, 60, 62);
        board.movePieceOrCapture(BitBoards::RB, 63, 61);
        board.forbidCastling(CastlingTypes::BlackKing);
        board.forbidCastling(CastlingTypes::BlackQueen);
    } else if (moveType == MoveTypes::CastleBlackQueen) {
        board.movePieceOrCapture(BitBoards::KB, 60, 58);
        board.movePieceOrCapture(BitBoards::RB, 56, 59);
        board.forbidCastling(CastlingTypes::BlackKing);
        board.forbidCastling(CastlingTypes::BlackQueen);
    } else {
        // non castle
        int fromSquare = move.from;
        int toSquare = move.to;
        U64 fromMask = 1ULL << fromSquare;
        U64 toMask = 1ULL << toSquare;

        // if castle piece is moved (or taken)
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_WHITE_KING) board.forbidCastling(CastlingTypes::WhiteKing);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_WHITE_QUEEN) board.forbidCastling(CastlingTypes::WhiteQueen);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_BLACK_KING) board.forbidCastling(CastlingTypes::BlackKing);
        if ((toMask | fromMask) & CASTLE_MASK_PIECES_BLACK_QUEEN) board.forbidCastling(CastlingTypes::BlackQueen);

        // exec normal move
        BitBoards bb = move.bb;
        isCapture = board.movePieceOrCapture(bb, fromSquare, toSquare);
        isPawnMove = bb == BitBoards::PW || bb == BitBoards::PB;

        bool isWhite = board.getSideToMove() == Side::White;

        // pawn specials
        if (moveType == MoveTypes::PawnDouble) {
            U64 nextEnpas = isWhite ? (fromMask << 8) : (fromMask >> 8);
            board.setEnpassantTarget(nextEnpas);
        } else if (moveType == MoveTypes::EnpasKing) {
            BitBoards otherPawns = isWhite ? BitBoards::PB : BitBoards::PW;
            board.removePiece(otherPawns, fromSquare + 1);
        } else if (moveType == MoveTypes::EnpasQueen) {
            BitBoards otherPawns = isWhite ? BitBoards::PB : BitBoards::PW;
            board.removePiece(otherPawns, fromSquare - 1);
        } else if (moveType == MoveTypes::Promote) {
            assert(move.promotion != MovePromotions::None);

            int pawnBoard = (int)BitBoards::PW;
            int promBoard = (int)BitBoards::PW;

            switch (move.promotion) {
                case MovePromotions::Q:
                    promBoard = (int)BitBoards::QW;
                    break;
                case MovePromotions::R:
                    promBoard = (int)BitBoards::RW;
                    break;
                case MovePromotions::N:
                    promBoard = (int)BitBoards::NW;
                    break;
                case MovePromotions::B:
                    promBoard = (int)BitBoards::BW;
                    break;
                case MovePromotions::None:
                    break;
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
        fullMovesCount++;
    }

    board.switchSide();

    // no-capture counter
    if (isCapture || isPawnMove) {
        noCaptureOrPush = 0;
    } else {
        noCaptureOrPush++;
    }
}

std::string Position::toFen() const {
    std::string fen = "";
    int emptyCount = 0;

    for (int j = 7; j >= 0; j--)  // reverse because fen starts at a8
    {
        for (int i = 0; i < 8; i++) {
            int piece = -1;
            for (int b = 0; b < 12; b++) {
                if (board.getBoard((BitBoards)b) & (1ULL << (8 * j + i))) {
                    piece = b;
                    break;
                }
            }
            if (piece >= 0) {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += "PRNBQKprnbqk"[piece];
            } else {
                emptyCount++;
            }
        }

        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
            emptyCount = 0;
        }

        if (j > 0) {
            fen += '/';
        }
    }

    if (board.getSideToMove() == Side::White) {
        fen += " w ";
    } else {
        fen += " b ";
    }

    size_t lastLen = fen.length();
    if (board.getCastlingRight(CastlingTypes::WhiteKing)) fen += "K";
    if (board.getCastlingRight(CastlingTypes::WhiteQueen)) fen += "Q";
    if (board.getCastlingRight(CastlingTypes::BlackKing)) fen += "k";
    if (board.getCastlingRight(CastlingTypes::BlackQueen)) fen += "q";

    if (fen.length() == lastLen) {
        fen += "- ";  // no castles
    }

    U64 enpasTarget = board.getEnpassantTarget();
    if (enpasTarget != 0) {
        int enpasSquare = trailingZeros(enpasTarget);
        fen += LanMove::squareIndexToString(enpasSquare);
    } else {
        fen += "-";
    }

    return fen;
}

void Position::print() {
    print(false);
}

void Position::print(bool moves) {
    std::cout << "\n";
    std::cout << "  -------------------\n";

    for (int j = 7; j >= 0; j--) {
        std::cout << (j + 1) << " | ";
        for (int i = 0; i < 8; i++) {
            U64 square = 1ull << (j * 8 + i);
            int b;
            for (b = 0; b < 12; b++) {
                if (board.getBoard((BitBoards)b) & square) {
                    break;
                }
            }
            std::cout << ("PRNBQKprnbqk."[b]) << " ";
        }
        std::cout << "|\n";
    }

    std::cout << "  -------------------\n";
    std::cout << "    a b c d e f g h  \n\n";

    std::cout << "Halfmove clock: " << noCaptureOrPush << "\n";
    std::cout << "Fullmove number: " << fullMovesCount << "\n\n";

    std::cout << "FEN: " << toFen() << "\n";
    std::cout << "Hash: " << std::hex << board.getHash() << std::dec << "\n\n";
    std::cout << "Is legal: " << board.isLegal() << std::endl;

    std::cout << "Checks: ";
    if (board.hasCheck(CheckFlags::WhiteInCheck)) std::cout << "white in check, ";
    if (board.hasCheck(CheckFlags::BlackInCheck)) std::cout << "black in check, ";
    std::cout << "\n\n";

    if (moves) {
        MoveList pseudoMoves;
        board.generatePseudoMoves(pseudoMoves);

        std::cout << "Pseudo moves: (" + std::to_string(pseudoMoves.size()) + ")\n";
        for (GenMove genMove : pseudoMoves) {
            std::cout << genMove.toString() << std::endl;
        }
        std::cout << "\n";
    }
}
