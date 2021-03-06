#include "Board.h"
#include "Debug.h"

#define U64 unsigned long long

// constructor
Board::Board(const U64 bitBoards[], const char sideToMove, const char castling, const char hasCastled,
    const U64 enpassantTarget, const U64 zobrist)
{
    for (int i = 0; i < 12; i++)
    {
        BitBoards[i] = bitBoards[i];
    }
    SideToMove = sideToMove;
    Castling = castling;
    HasCastled = hasCastled;
    EnpassantTarget = enpassantTarget;
    Zobrist = zobrist;

    Init();
}

Board Board::Default()
{
    std::vector<std::string> fen = { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1" };
    return FromFEN(fen, 0);
}

/**
* Make move and return new updated board
* Automatically change Zobrist hash with new changes instead of creating new one from scratch
*/
Board Board::Move(const int move) const
{
    Board newBoard(*this);
    newBoard.LastBoard = this;

    bool captureOrPush = false;

    ////////////////////////////// MOVE ////////////////////////////////////
    if (move & MOVE_TYPE_CASTLE)
    {
        if (newBoard.SideToMove == WHITE_TO_MOVE)
        {
            if (move & MOVE_INFO_CASTLE_KINGSIDE)
            {
                newBoard.ExecuteMove(KW, 4, 6); // set king to g1
                newBoard.ExecuteMove(RW, 7, 5); // switch rook to f1
                newBoard.HasCastled |= WHITE_HAS_CASTLED;
            }
            else
            {
                newBoard.ExecuteMove(KW, 4, 2); // set king to c1
                newBoard.ExecuteMove(RW, 0, 3); // switch rook to d1
                newBoard.HasCastled |= WHITE_HAS_CASTLED;
            }

            newBoard.ForbidCastling(0);
            newBoard.ForbidCastling(1);
        }
        else
        {
            if (move & MOVE_INFO_CASTLE_KINGSIDE)
            {
                newBoard.ExecuteMove(KB, 60, 62);
                newBoard.ExecuteMove(RB, 63, 61);
                newBoard.HasCastled |= BLACK_HAS_CASTLED;
            }
            else
            {
                newBoard.ExecuteMove(KB, 60, 58);
                newBoard.ExecuteMove(RB, 56, 59);
                newBoard.HasCastled |= BLACK_HAS_CASTLED;
            }

            newBoard.ForbidCastling(2);
            newBoard.ForbidCastling(3);
        }
    }
    else // not a castle
    {
        int fromIndex = move & 0xFF;
        int toIndex = (move >> 8) & 0xFF;
        U64 from = 1ULL << fromIndex;
        U64 to = 1ULL << toIndex;

        // if castle piece is moved (or eaten)
        if ((to | from) & CASTLE_MASK_PIECES_WHITE_KING)  newBoard.ForbidCastling(0);
        if ((to | from) & CASTLE_MASK_PIECES_WHITE_QUEEN) newBoard.ForbidCastling(1);
        if ((to | from) & CASTLE_MASK_PIECES_BLACK_KING)  newBoard.ForbidCastling(2);
        if ((to | from) & CASTLE_MASK_PIECES_BLACK_QUEEN) newBoard.ForbidCastling(3);

        // Find board which moves piece
        for (int i = 0; i < 12; i++)
        {
            if (newBoard.BitBoards[i] & from)
            {
                bool isCapture = newBoard.ExecuteMove(i, fromIndex, toIndex);

                if (isCapture || i == PW || i == PB)
                {
                    captureOrPush = true;
                }

                break;
            }
        }

        // SPECIALS
        if (move & MOVE_TYPE_PAWN_HOPP)
        {
            if (newBoard.SideToMove == WHITE_TO_MOVE)
                newBoard.SetEnpassantTarget(from << 8);
            else
                newBoard.SetEnpassantTarget(from >> 8);
        }
        else if (move & MOVE_TYPE_ENPASSANT)
        {
            // KILL HOPPED PAWN
            if (newBoard.SideToMove == WHITE_TO_MOVE) // white
            {
                U64 victim = 0;
                if (move & MOVE_INFO_ENPASSANT_LEFT)
                    victim = from >> 1;
                else
                    victim = from << 1;

                newBoard.BitBoards[PB] &= ~victim; // kill piece
                newBoard.Zobrist ^= ZobristValues[PB * 64 + trailingZeros(victim)];
            }
            else
            {
                U64 victim = 0;
                if (move & MOVE_INFO_ENPASSANT_LEFT)
                    victim = from << 1;
                else
                    victim = from >> 1;

                newBoard.BitBoards[PW] &= ~victim; // kill piece
                newBoard.Zobrist ^= ZobristValues[PW * 64 + trailingZeros(victim)];
            }
        }
        else if (move & MOVE_TYPE_PROMOTE)
        {
            if (newBoard.SideToMove == WHITE_TO_MOVE)
            {
                newBoard.BitBoards[PW] &= ~to; // turn off bit
                newBoard.Zobrist ^= ZobristValues[PW * 64 + toIndex];
                if (move & MOVE_INFO_PROMOTE_QUEEN)
                {
                    newBoard.BitBoards[QW] |= to; newBoard.Zobrist ^= ZobristValues[QW * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_HORSEY)
                {
                    newBoard.BitBoards[NW] |= to; newBoard.Zobrist ^= ZobristValues[NW * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_ROOK)
                {
                    newBoard.BitBoards[RW] |= to; newBoard.Zobrist ^= ZobristValues[RW * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_BISHOP)
                {
                    newBoard.BitBoards[BW] |= to; newBoard.Zobrist ^= ZobristValues[BW * 64 + toIndex];
                }
            }
            else
            {
                newBoard.BitBoards[PB] &= ~to; // turn off bit
                newBoard.Zobrist ^= ZobristValues[PB * 64 + toIndex];
                if (move & MOVE_INFO_PROMOTE_QUEEN)
                {
                    newBoard.BitBoards[QB] |= to; newBoard.Zobrist ^= ZobristValues[QB * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_HORSEY)
                {
                    newBoard.BitBoards[NB] |= to; newBoard.Zobrist ^= ZobristValues[NB * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_ROOK)
                {
                    newBoard.BitBoards[RB] |= to; newBoard.Zobrist ^= ZobristValues[RB * 64 + toIndex];
                }
                else if (move & MOVE_INFO_PROMOTE_BISHOP)
                {
                    newBoard.BitBoards[BB] |= to; newBoard.Zobrist ^= ZobristValues[BB * 64 + toIndex];
                }
            }
        }
    }

    if ( !(move & MOVE_TYPE_PAWN_HOPP) )
    {
        newBoard.SetEnpassantTarget(0);
    }

    // count fullMove counter
    if (newBoard.SideToMove == BLACK_TO_MOVE)
    {
        newBoard.FullMovesCount++;
    }

    // switch player
    newBoard.SideToMove ^= 1;
    newBoard.Zobrist ^= ZobristValues[ZOBRISTTABLE_BLACK_MOVE];

    // no-capture counter
    if (captureOrPush)
    {
        newBoard.NoCaptureOrPush = 0;
    }
    else
    {
        newBoard.NoCaptureOrPush++;
    }

    newBoard.Init();


    return newBoard;
}

bool Board::ExecuteMove(int bb, int from, int to)
{
    U64 fromMask = 1ULL << from;
    U64 toMask = 1ULL << to;

    bool isCapture = false;

    // capture or not?
    if (toMask & Occupied) 
    {
        for (int i = 0; i < 12; i++)
        {
            if (toMask & BitBoards[i])
            {
                // turn captured bit off
                BitBoards[i] &= ~toMask;
                Zobrist ^= ZobristValues[i * 64 + to];
                
                isCapture = true;
                break; // only one capture possible
            }
        }
    }

    // MOVE PIECE
    // turn last bit off
    BitBoards[bb] &= ~fromMask;
    Zobrist ^= ZobristValues[bb * 64 + from];
    // turn new bit on
    BitBoards[bb] |= toMask;
    Zobrist ^= ZobristValues[bb * 64 + to];

    return isCapture;
}

void Board::ForbidCastling(int castlingType)
{
    int mask = 1 << castlingType;
    if (Castling & mask)
    {
        Castling &= ~mask;
        Zobrist ^= ZobristValues[ZOBRISTTABLE_CASTLING + castlingType];
    }
}

void Board::SetEnpassantTarget(U64 newTarget)
{
    // xor old (if not 0)
    if (EnpassantTarget)
    {
        int enpassantIndex = ZOBRISTTABLE_ENPASSANT + trailingZeros(EnpassantTarget);
        Zobrist ^= ZobristValues[enpassantIndex];
    }

    EnpassantTarget = newTarget;
    
    // xor new (if not 0)
    if (EnpassantTarget)
    {
        int enpassantIndex = ZOBRISTTABLE_ENPASSANT + trailingZeros(EnpassantTarget);
        Zobrist ^= ZobristValues[enpassantIndex];
    }
}

bool Board::IsLegal() const
{
    bool whiteCheck = Checks & CHECK_WHITE;
    bool blackCheck = Checks & CHECK_BLACK;

    if (SideToMove == BLACK_TO_MOVE && whiteCheck) return false; // illegal
    if (SideToMove == WHITE_TO_MOVE && blackCheck) return false;

    return true;
}

bool Board::IsStalemate() const
{
    // 50 move rule
    if (NoCaptureOrPush >= 50) return true;

    // threefold rep.
    int numberOfRepetitions = 1;

    const Board* board = this->LastBoard; 
    for (int i = 0; i < 100; i++)
    {
        if (board == nullptr)
        {
            break;
        }

        /*
        *   Detect same position based on Zobrist hash
        *   Not ideal, enpassant square etc. are also in hash, 
        *   therefore position where enpassant could be made is not seen as equal.
        *   but enpassant can kiss my ass so idc
        */
        if (Zobrist == board->Zobrist)
        {
            numberOfRepetitions++;
        }

        if (numberOfRepetitions >= 3)
        {
            return true;
        }

        if (board->NoCaptureOrPush == 0)
        {
            break;
        }

        board = board->LastBoard;

        if (board == board->LastBoard || board == nullptr)
        {
            break;
        }

        if (i == 99)
        {
            LOG("IsStalemate() has run for 100 iterations (probably loop in lastBoard pointers)");
        }
    }

    //material
    bool sufficientMaterial = false;
    if (BitBoards[PW] | BitBoards[RW] | BitBoards[QW] | BitBoards[PB] | BitBoards[RB] | BitBoards[QB])
    {
        // at least one winning piece
        sufficientMaterial = true;
    }
    else if (countBits(BitBoards[BW]) > 1 || countBits(BitBoards[BB]) > 1)
    {
        // at least two bishops
        sufficientMaterial = true;
    }
    else if ((BitBoards[BW] != 0 && BitBoards[NW] != 0) || (BitBoards[BB] != 0 && BitBoards[NB] != 0))
    {
        // one or more bishops and one or more knights
        sufficientMaterial = true;
    }

    return !sufficientMaterial;
}

void Board::Init()
{
    // UPDATE VARIOUS BITBOARDS
    WhitePieces = BitBoards[PW] | BitBoards[RW] | BitBoards[NW] | BitBoards[BW] | BitBoards[QW];
    BlackPieces = BitBoards[PB] | BitBoards[RB] | BitBoards[NB] | BitBoards[BB] | BitBoards[QB];
    NotWhitePieces = ~(WhitePieces | BitBoards[KW] | BitBoards[KB]);
    NotBlackPieces = ~(BlackPieces | BitBoards[KW] | BitBoards[KB]);
    Empty = NotWhitePieces & NotBlackPieces;
    Occupied = ~Empty;

    // FIND UNSAFE SQUARES
    FindUnsafeForBlack();
    FindUnsafeForWhite();

    Checks = 0;
    // CHECK CHECK
    if (BitBoards[KW] & UnsafeForWhite)
        Checks |= CHECK_WHITE;
    else if (BitBoards[KB] & UnsafeForBlack)
        Checks |= CHECK_BLACK;
}

void Board::GenerateZobrist()
{
    Zobrist = 0ULL;
    for (int i = 0; i < 64; i++)
    {
        // pieces
        U64 mask = (1ULL << i);
        for (int n = 0; n < 12; n++)
        {
            if (BitBoards[n] & mask)
            {
                int index = ZOBRISTTABLE_PIECES + 64 * n + i;
                Zobrist ^= ZobristValues[index];
            }
        }
    }

    // enpassant
    if (EnpassantTarget)
    {
        int enpassantIndex = ZOBRISTTABLE_ENPASSANT + trailingZeros(EnpassantTarget);
        Zobrist ^= ZobristValues[enpassantIndex];
    }

    // turnright
    if (SideToMove == BLACK_TO_MOVE)
    {
        Zobrist ^= ZobristValues[ZOBRISTTABLE_BLACK_MOVE];
    }

    // castling
    for (int i = 0; i < 4; i++)
    {
        if (Castling & (1 << i))
        {
            Zobrist ^= ZobristValues[ZOBRISTTABLE_CASTLING + i];
        }
    }
}

void Board::FindUnsafeForWhite()
{
    U64 unsafe = 0;
    // pawns
    unsafe |= (BitBoards[PB] >> 7) & ~FILE_A;
    unsafe |= (BitBoards[PB] >> 9) & ~FILE_H;

    // BISHOPS
    int i = 0;
    U64 bishops = BitBoards[BB];
    while (bishops)
    {
        i = trailingZeros(bishops);
        bishops ^= 1ULL << i; // unset this bit
        U64 bishopMoves = DandAntiDMoves(i);
        unsafe |= bishopMoves;
    }
    // ROOKS
    i = 0;
    U64 rooks = BitBoards[RB];
    while (rooks)
    {
        i = trailingZeros(rooks);
        rooks ^= 1ULL << i; // unset this bit
        unsafe |= HAndVMoves(i);
    }
    // QUEENS
    i = 0;
    U64 queens = BitBoards[QB];
    while (queens)
    {
        i = trailingZeros(queens);
        queens ^= 1ULL << i; // unset this bit
        unsafe |= (HAndVMoves(i) | DandAntiDMoves(i));
    }
    // HORSES
    i = 0;
    U64 horses = BitBoards[NB];
    while (horses)
    {
        i = trailingZeros(horses);
        horses ^= 1ULL << i; // unset this bit
        int offset = i - SPAN_HORSE_OFFSET;
        U64 horseMoves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  horseMoves &= ~FILE_GH;
        else            horseMoves &= ~FILE_AB;
        unsafe |= horseMoves;
    }
    // KING
    i = 0;
    U64 king = BitBoards[KB];
    if (king)
    {
        i = trailingZeros(king);
        king ^= 1ULL << i; // unset this bit
        int offset = i - SPAN_KING_OFFSET;
        U64 kingMoves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  kingMoves &= ~FILE_H;
        else            kingMoves &= ~FILE_A;
        unsafe |= kingMoves;
    }

    UnsafeForWhite = unsafe;
}

void Board::FindUnsafeForBlack()
{
    U64 unsafe = 0;
    // pawns
    unsafe |= (BitBoards[PW] << 7) & ~FILE_H;
    unsafe |= (BitBoards[PW] << 9) & ~FILE_A;

    // BISHOPS
    int i = 0;
    U64 bishops = BitBoards[BW];
    while (bishops)
    {
        i = trailingZeros(bishops);
        bishops ^= 1ULL << i; // unset this bit
        U64 bishopMoves = DandAntiDMoves(i);
        unsafe |= bishopMoves;
    }
    // ROOKS
    i = 0;
    U64 rooks = BitBoards[RW];
    while (rooks)
    {
        i = trailingZeros(rooks);
        rooks ^= 1ULL << i; // unset this bit
        unsafe |= HAndVMoves(i);
    }
    // QUEENS
    i = 0;
    U64 queens = BitBoards[QW];
    while (queens)
    {
        i = trailingZeros(queens);
        queens ^= 1ULL << i; // unset this bit
        unsafe |= (HAndVMoves(i) | DandAntiDMoves(i));
    }
    // HORSES
    i = 0;
    U64 horses = BitBoards[NW];
    while (horses)
    {
        i = trailingZeros(horses);
        horses ^= 1ULL << i; // unset this bit
        int offset = i - SPAN_HORSE_OFFSET;
        U64 horseMoves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  horseMoves &= ~FILE_GH;
        else            horseMoves &= ~FILE_AB;
        unsafe |= horseMoves;
    }
    // KING
    i = 0;
    U64 king = BitBoards[KW];
    if (king)
    {
        i = trailingZeros(king);
        king ^= 1ULL << i; // unset this bit
        int offset = i - SPAN_KING_OFFSET;
        U64 kingMoves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  kingMoves &= ~FILE_H;
        else            kingMoves &= ~FILE_A;
        unsafe |= kingMoves;
    }

    UnsafeForBlack = unsafe;
}

#undef U64