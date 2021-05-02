#include <iostream>
#include <regex>

#include "Board.h"

#define U64 unsigned long long

std::string Board::MoveToText(int m, bool onlyMove)
{
    int from = m & 0xFF; // snap off rest
    int to = (m >> 8) & 0xFF; // same;
    std::string msg = IndexToText(from) + IndexToText(to);

    if (m & MOVE_TYPE_PROMOTE)
    {
        if (m & MOVE_INFO_PROMOTE_QUEEN)
            msg += 'q';
        else if (m & MOVE_INFO_PROMOTE_ROOK)
            msg += 'r';
        else if (m & MOVE_INFO_PROMOTE_HORSEY)
            msg += 'n';
        else if (m & MOVE_INFO_PROMOTE_BISHOP)
            msg += 'b';
    }

    if (!onlyMove)
    {
        if (m & MOVE_TYPE_PAWN_HOPP)
            msg += " type: Pawn hopp";
        else if (m & MOVE_TYPE_CASTLE)
        {
            msg += " type: Castle";
            if (m & MOVE_INFO_CASTLE_KINGSIDE)
                msg += " kingside";
            else if (m & MOVE_INFO_CASTLE_QUEENSIDE)
                msg += " queenside";
            else
                msg += " NO CASTLE DIRECTION GIVEN!";
        }
        else if (m & MOVE_TYPE_ENPASSANT)
        {
            msg += " type: Enpassant";
            if (m & MOVE_INFO_ENPASSANT_LEFT)
                msg += " to left";
            else if (m & MOVE_INFO_ENPASSANT_RIGHT)
                msg += " to right";
            else
                msg += " NO ENPASSANT DIRECTION GIVEN!";
        }
        else if (m & MOVE_TYPE_PROMOTE)
        {
            msg += " type: Promotion";
            if (m & MOVE_INFO_PROMOTE_QUEEN)
                msg += ", (Queen)";
            else if (m & MOVE_INFO_PROMOTE_ROOK)
                msg += ", (Rook)";
            else if (m & MOVE_INFO_PROMOTE_HORSEY)
                msg += ", (Horsey)";
            else if (m & MOVE_INFO_PROMOTE_BISHOP)
                msg += ", (Bishop)";
            else
                msg += " NO PIECE SPECIFIED!";
        }
    }

    return msg;
}

std::string Board::IndexToText(int index)
{
    return "abcdefgh"[index % 8] + std::to_string(index / 8 + 1);
}

int Board::TextToIndex(const std::string& text)
{
    if (text.length() < 2)
    {
        return -1;
    }

    std::string lowerCase(text); 
    for (int i = 0; lowerCase[i] != '\0'; i++)
    {
        if (lowerCase[i] >= 'A' && lowerCase[i] <= 'Z')
        {
            lowerCase[i] = lowerCase[i] + 'a' - 'A';
        }
    }
    
    //           file starting at 'a'      rank starting at '1' * 8
    int index = (int)lowerCase[0] - 'a' + (int)(lowerCase[1] - '1') * 8;
    if (index < 0 || index > 63)
        return -1;
    return index;
}

Board Board::FromFEN(const std::vector<std::string> &arguments, int startIndex)
{
    //std::regex regular_exp("((^|\\/)[PRBNQKprbnqk\\d]+){8}\\s[WwBb]\\s([KQkq]{1,4}|\\-)\\s(([AaBbCcDdEeFfGgHh]\\d)|\\-)");
    //std::smatch sm;
    //std::regex_search(fenInput, sm, regular_exp);
    //if (sm.size() == 0)
    //    return Board();
    //std::string fen = sm[0];

    Board board = Board();

    int argumentIndex = startIndex;

    // only so that i can break out of block
    while (true)
    {
        if (argumentIndex >= arguments.size()) break;
        const std::string& piecePositions = arguments[argumentIndex];
        argumentIndex++;

        // PIECE POSITIONS
        int i = 0, j = 7; // decrease j so that LSB is A0 and not A7 like in FEN
        char srchString[] = "PRNBQKprnbqk12345678/";
        int strIndex = 0;
        while (true)
        {
            const char* fenChar = strchr(srchString, piecePositions[strIndex++]);
            if (fenChar == NULL)
            {
                break; // end of piece description
            }

            int charIndex = fenChar - srchString;
            if (charIndex < 12) // letter
            {
                if (i < 8) // safety if too many chars
                {
                    // piece hit, set bit
                    board.BitBoards[charIndex] |= 1ULL << (8 * j + i);
                    i++;
                }
            }
            else if (charIndex < 20) // number
            {
                i += charIndex - 11;
            }
            else // '/'
            {
                i = 0; j--;
                if (j < 0)
                {
                    break;
                }
            }
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& sideToMove = arguments[argumentIndex];
        argumentIndex++;

        // SideToMove
        if (sideToMove == "w")
            board.SideToMove = WHITE_TO_MOVE;
        else if (sideToMove == "b")
            board.SideToMove = BLACK_TO_MOVE;
        else
        {
            break;
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& castling = arguments[argumentIndex];
        argumentIndex++;

        // Castling
        for (int i = 0; i < castling.length(); i++)
        {
            char ca = castling[i];

            switch (ca)
            {
            case 'K':
                board.Castling |= CASTLE_KW;
                break;
            case 'Q':
                board.Castling |= CASTLE_QW;
                break;
            case 'k':
                board.Castling |= CASTLE_KB;
                break;
            case 'q':
                board.Castling |= CASTLE_QB;
                break;
            }
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& enpas = arguments[argumentIndex];
        argumentIndex++;

        // enpassant square
        int square = Board::TextToIndex(enpas);
        if (square >= 0 && square < 64)
        {
            board.EnpassantTarget = 1ULL << square;
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& halfMoveClock = arguments[argumentIndex];
        argumentIndex++;

        // halfmoveclocl
        int hmc = std::stoi(halfMoveClock);
        if (hmc > 0)
        {
            board.NoCaptureOrPush = hmc;
        }

        if (argumentIndex >= arguments.size()) break;
        const std::string& fullMoves = arguments[argumentIndex];
        argumentIndex++;

        // full moves
        int fm = std::stoi(fullMoves);
        if (fm > 0)
        {
            board.FullMovesCount = fm;
        }

        break;
    }

    board.GenerateZobrist();
    board.Init();
    return board;
}

std::string Board::ToFEN() const
{
    std::string fen = "";
    int emptyCount = 0;
    for (int j = 7; j >= 0; j--) // reverse because fen starts at a8
    {
        for (int i = 0; i < 8; i++)
        {
            int piece = -1;
            for (int b = 0; b < 12; b++)
            {
                if (BitBoards[b] & (1ULL << (8 * j + i)))
                {
                    piece = b;
                    break;
                }
            }
            if (piece >= 0)
            {
                if (emptyCount > 0)
                {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += "PRNBQKprnbqk"[piece];
            }
            else
            {
                emptyCount++;
            }
        }

        if (emptyCount > 0)
        {
            fen += std::to_string(emptyCount);
            emptyCount = 0;
        }

        if (j > 0)
        {
            fen += '/';
        }
    }

    if (SideToMove == WHITE_TO_MOVE)
        fen += " w ";
    else
        fen += " b ";

    if (Castling)
    {
        if (Castling & CASTLE_KW) fen += "K";
        if (Castling & CASTLE_QW) fen += "Q";
        if (Castling & CASTLE_KB) fen += "k";
        if (Castling & CASTLE_QB) fen += "q";
        fen += " ";
    }
    else
        fen += "- ";

    if (EnpassantTarget)
        fen += IndexToText(trailingZeros(EnpassantTarget));
    else
        fen += "-";

    return fen;
}

void Board::Print() const
{
    std::cout << "\n";
    std::cout << "  -------------------\n";

    for (int j = 7; j >= 0; j--)
    {
        std::cout << (j + 1) << " | ";
        for (int i = 0; i < 8; i++)
        {
            int index = j * 8 + i;
            int b;
            for (b = 0; b < 12; b++)
            {
                if (BitBoards[b] & 1ULL << index)
                    break;
            }

            std::cout << ("PRNBQKprnbqk."[b]) << " ";
        }
        std::cout << "|\n";
    }

    std::cout << "  -------------------\n";
    std::cout << "    a b c d e f g h  \n\n";

    std::cout << "Halfmove clock: " << NoCaptureOrPush << "\n";
    std::cout << "Fullmove number: " << FullMovesCount << "\n\n";

    std::cout << "FEN: " << ToFEN() << "\n";
    std::cout << "Hash: " << std::hex << Zobrist << std::dec << "\n\n";
}

void Board::PrintAllBitBoards() const
{
    std::string names[] =
    {
        "White Pawns", "White Rooks", "White Knights", "White Bishops", "White Queen", "White King",
        "Black Pawns", "Black Rooks", "Black Knights", "Black Bishops", "Black Queen", "Black King"
    };

    for (int i = 0; i < 12; i++)
    {
        std::cout << names[i] << std::endl;
        PrintBitBoard(BitBoards[i]);
    }
}

void Board::PrintBitBoard(const U64& board)
{
    std::cout << "\n  -------------------\n";

    for (int j = 7; j >= 0; j--)
    {
        std::cout << (j + 1) << " | ";
        for (int i = 0; i < 8; i++)
        {
            int index = j * 8 + i;
            if (board & 1ULL << index)
                std::cout << "X ";
            else
                std::cout << ". ";
        }
        std::cout << "|\n";
    }

    std::cout << "  -------------------\n";
    std::cout << "    a b c d e f g h  \n\n";
}

#undef U64