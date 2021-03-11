#include "Board.h"
#include "Evaluation.h"
#include "algorithm"

#define U64 unsigned long long

/**
 * GENERATES LEGAL MOVES (SLOW!)
 */
void Board::GenerateLegalMoves(std::vector<MOVE>& moveList) const
{
    std::vector<MOVE> pseudoMoves;
    pseudoMoves.reserve(40);
    GeneratePseudoMoves(pseudoMoves);
    moveList.reserve(pseudoMoves.size()); // less or equal length than pseudomoves
    for (const MOVE& m : pseudoMoves)
    {
        Board testBoard = Move(m.second);
        if (testBoard.GetBoardStatus())
            moveList.push_back(m);
    }
}

/**
* GENERATES PSEUDO MOVES (SOME MIGHT BE ILLEGAL, LIKE DISCOVERY ATTACKS ON KING).
* IF CURRENT POSITION IS ILLEGAL, 0 IS RETURNED
*/
void Board::GeneratePseudoMoves(std::vector<MOVE>& moveList) const
{
    std::vector<MOVE> captures;
    captures.reserve(20);
    std::vector<MOVE> nonCaptures;
    nonCaptures.reserve(40);

    bool isWhite = SideToMove == WHITE_TO_MOVE;

    KnightMoves(captures, nonCaptures, isWhite); // horseys
    KingMoves(captures, nonCaptures, isWhite); // king

    if (isWhite)
    {
        MoveSlidingPiece(captures, nonCaptures, BitBoards[BW], false, true, true); // bishops
        MoveSlidingPiece(captures, nonCaptures, BitBoards[RW], true, false, true); // rooks
        MoveSlidingPiece(captures, nonCaptures, BitBoards[QW], true, true, true); // queens
        PawnMovesWhite(captures, nonCaptures); // pawns
        CastlesWhite(captures, nonCaptures); // castles
    }
    else
    {
        MoveSlidingPiece(captures, nonCaptures, BitBoards[BB], false, true, false); // bishops
        MoveSlidingPiece(captures, nonCaptures, BitBoards[RB], true, false, false); // rooks
        MoveSlidingPiece(captures, nonCaptures, BitBoards[QB], true, true, false); // queens
        PawnMovesBlack(captures, nonCaptures); // pawns
        CastlesBlack(captures, nonCaptures); // castles
    };

    // LEAST VALUABLE ATTACKER - MOST VALUABLE VICTIM
    for (MOVE& m : captures)
    {
        // add value of victim
        int offset = isWhite ? 6 : 0;
        for (int i = offset; i < 5 + offset; i++)
        {
            int to = (m.second >> 8) & 0xFF;
            if (BitBoards[i] & (1ULL << to))
            {
                m.first += PIECE_VALUES[i % 6];
            }
        }
        // subtract value of attacker
        offset = isWhite ? 0 : 6;
        for (int i = offset; i < 5 + offset; i++)
        {
            int from = m.second & 0xFF;
            if (BitBoards[i] & (1ULL << from))
            {
                m.first -= PIECE_VALUES[i % 6];
            }
        }
    }

    // SORT CAPTURES
    std::stable_sort(captures.begin(), captures.end(), [](const MOVE& a, const MOVE& b) {
        return a.first > b.first;
    });

    // RESERVE SPACE
    moveList.reserve(captures.size() + nonCaptures.size());
    // INSERT CAPTURES
    moveList.insert(moveList.end(), captures.begin(), captures.end());
    // INSERT NON-CAPTURES
    moveList.insert(moveList.end(), nonCaptures.begin(), nonCaptures.end());
}

void Board::GenerateCaptures(std::vector<MOVE>& captures) const
{
    captures.reserve(20);

    bool isWhite = SideToMove == WHITE_TO_MOVE;

    KnightMoves(captures, isWhite); // horseys
    KingMoves(captures, isWhite); // king

    if (isWhite)
    {
        MoveSlidingPiece(captures, BitBoards[BW], false, true, true); // bishops
        MoveSlidingPiece(captures, BitBoards[RW], true, false, true); // rooks
        MoveSlidingPiece(captures, BitBoards[QW], true, true, true); // queens
        PawnMovesWhite(captures); // pawns
        // castles not needed
    }
    else
    {
        MoveSlidingPiece(captures, BitBoards[BB], false, true, false); // bishops
        MoveSlidingPiece(captures, BitBoards[RB], true, false, false); // rooks
        MoveSlidingPiece(captures, BitBoards[QB], true, true, false); // queens
        PawnMovesBlack(captures); // pawns
        // castles not needed
    };
}

void Board::CastlesWhite(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const
{
    if (Castling & CASTLE_KW) // CASTLE KINGSIDE STILL ALLOWED?
    {
        bool lineUnderAttack = UnsafeForWhite & CASTLE_MASK_W_K_PATH; // KINS PATH UNDER ATTACK?
        bool obstructed = Occupied & CASTLE_MASK_W_K_GAP; // ANY PIECE INBETWEEN?
        if (!lineUnderAttack && !obstructed)
        {
            int m = (6 << 8) | 4 | MOVE_TYPE_CASTLE | MOVE_INFO_CASTLE_KINGSIDE;
            nonCaptures.push_back(MOVE(0, m)); // add move
        }
    }
    if (Castling & CASTLE_QW) // CASTLE QUEENSIDE STILL ALLOWED?
    {
        bool lineUnderAttack = UnsafeForWhite & CASTLE_MASK_W_Q_PATH; // KINS PATH UNDER ATTACK?
        bool obstructed = Occupied & CASTLE_MASK_W_Q_GAP; // ANY PIECE INBETWEEN?
        if (!lineUnderAttack && !obstructed)
        {
            int m = (2 << 8) | 4 | MOVE_TYPE_CASTLE | MOVE_INFO_CASTLE_QUEENSIDE;
            nonCaptures.push_back(MOVE(0, m)); // add move
        }
    }
}

void Board::CastlesBlack(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const
{
    if (Castling & CASTLE_KB) // CASTLE KINGSIDE STILL ALLOWED?
    {
        bool lineUnderAttack = UnsafeForBlack & CASTLE_MASK_B_K_PATH; // KINS PATH UNDER ATTACK?
        bool obstructed = Occupied & CASTLE_MASK_B_K_GAP; // ANY PIECE INBETWEEN?
        if (!lineUnderAttack && !obstructed)
        {
            int m = (62 << 8) | 60 | MOVE_TYPE_CASTLE | MOVE_INFO_CASTLE_KINGSIDE; // add move
            nonCaptures.push_back(MOVE(0, m));
        }
    }
    if (Castling & CASTLE_QB) // CASTLE QUEENSIDE STILL ALLOWED?
    {
        bool lineUnderAttack = UnsafeForBlack & CASTLE_MASK_B_Q_PATH; // KINS PATH UNDER ATTACK?
        bool obstructed = Occupied & CASTLE_MASK_B_Q_GAP; // ANY PIECE INBETWEEN?
        if (!lineUnderAttack && !obstructed)
        {
            int m = (58 << 8) | 60 | MOVE_TYPE_CASTLE | MOVE_INFO_CASTLE_QUEENSIDE; // add move
            nonCaptures.push_back(MOVE(0, m));
        }
    }
}

void Board::MoveSlidingPiece(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, U64 bitboard, bool paral, bool diag, bool isWhite) const
{
    const U64& validPositions = isWhite ? NotWhitePieces : NotBlackPieces;
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    int i = 0;
    while (bitboard)
    {
        i = trailingZeros(bitboard);
        bitboard ^= 1ULL << i; // unset this bit
        // -> piece at i
        U64 moves = 0;
        // find all moves
        if (paral) moves |= HAndVMoves(i);
        if (diag) moves |= DandAntiDMoves(i);
        // mask with board
        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
        AddMovesFromBitboard(nonCaptures, moves & ~othersPieces, i, 0);
    }
}

void Board::MoveSlidingPiece(std::vector<MOVE>& captures, U64 bitboard, bool paral, bool diag, bool isWhite) const
{
    const U64& validPositions = isWhite ? NotWhitePieces : NotBlackPieces;
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    int i = 0;
    while (bitboard)
    {
        i = trailingZeros(bitboard);
        bitboard ^= 1ULL << i; // unset this bit
        // -> piece at i
        U64 moves = 0;
        // find all moves
        if (paral) moves |= HAndVMoves(i);
        if (diag) moves |= DandAntiDMoves(i);
        // mask with board
        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
    }
}

U64 Board::HAndVMoves(int index) const
{
    U64 s = 1ULL << index;
    int i = index % 8, j = index / 8;

    // find all moves by magic
    U64 horizontal = (Occupied - 2 * s) ^ reverse(reverse(Occupied) - 2 * reverse(s));
    U64 vertical = ((Occupied & FILE_MASKS[i]) - 2 * s) ^ reverse(reverse(Occupied & FILE_MASKS[i]) - 2 * reverse(s));
    return (horizontal & RANK_MASKS[j]) | (vertical & FILE_MASKS[i]);
}

U64 Board::DandAntiDMoves(int index) const
{
    U64 s = 1ULL << index;
    int d = (index / 8) + (index % 8);
    int ad = (index / 8) + 7 - (index % 8);

    // find all moves by magic
    U64 diag = ((Occupied & DIAG_MASK[d]) - 2 * s) ^ reverse(reverse(Occupied & DIAG_MASK[d]) - 2 * reverse(s));
    U64 antiDiag = ((Occupied & ANTIDIAG_MASK[ad]) - 2 * s) ^ reverse(reverse(Occupied & ANTIDIAG_MASK[ad]) - 2 * reverse(s));
    return (diag & DIAG_MASK[d]) | (antiDiag & ANTIDIAG_MASK[ad]);
}

void Board::KingMoves(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, bool isWhite) const
{
    U64 king = isWhite ? BitBoards[KW] : BitBoards[KB];
    U64 validPositions = isWhite ? (NotWhitePieces & ~UnsafeForWhite) : (NotBlackPieces & ~UnsafeForBlack);
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    while (king)
    {
        int i = trailingZeros(king);
        king ^= 1ULL << i; // somehow necessary if multiple kings...
        // -> piece at i
        int offset = i - SPAN_KING_OFFSET;
        U64 moves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  moves &= ~FILE_H;
        else            moves &= ~FILE_A;
        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
        AddMovesFromBitboard(nonCaptures, moves & ~othersPieces, i, 0);
    }
}

void Board::KingMoves(std::vector<MOVE>& captures, bool isWhite) const
{
    U64 king = isWhite ? BitBoards[KW] : BitBoards[KB];
    U64 validPositions = isWhite ? (NotWhitePieces & ~UnsafeForWhite) : (NotBlackPieces & ~UnsafeForBlack);
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    while (king)
    {
        int i = trailingZeros(king);
        king ^= 1ULL << i; // somehow necessary if multiple kings...
        // -> piece at i
        int offset = i - SPAN_KING_OFFSET;
        U64 moves = offset > 0 ? SPAN_KING << offset : SPAN_KING >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  moves &= ~FILE_H;
        else            moves &= ~FILE_A;
        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
    }
}

void Board::KnightMoves(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, bool isWhite) const
{
    U64 horse = isWhite ? BitBoards[NW] : BitBoards[NB];
    const U64& validPositions = isWhite ? NotWhitePieces : NotBlackPieces;
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    int i = 0;
    while (horse)
    {
        i = trailingZeros(horse);
        horse ^= 1ULL << i; // unset this bit
        // -> piece at i
        int offset = i - SPAN_HORSE_OFFSET;
        U64 moves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  moves &= ~FILE_GH;
        else            moves &= ~FILE_AB;
        
        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
        AddMovesFromBitboard(nonCaptures, moves & ~othersPieces, i, 0);
    }
}

void Board::KnightMoves(std::vector<MOVE>& captures, bool isWhite) const
{
    U64 horse = isWhite ? BitBoards[NW] : BitBoards[NB];
    const U64& validPositions = isWhite ? NotWhitePieces : NotBlackPieces;
    const U64& othersPieces = isWhite ? BlackPieces : WhitePieces;

    int i = 0;
    while (horse)
    {
        i = trailingZeros(horse);
        horse ^= 1ULL << i; // unset this bit
        // -> piece at i
        int offset = i - SPAN_HORSE_OFFSET;
        U64 moves = offset > 0 ? SPAN_HORSE << offset : SPAN_HORSE >> -offset; // weird stuff happens when shifting by neg number
        if (i % 8 < 4)  moves &= ~FILE_GH;
        else            moves &= ~FILE_AB;

        moves &= validPositions;
        // add to list
        AddMovesFromBitboard(captures, moves & othersPieces, i, 0);
    }
}

void Board::PawnMovesWhite(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const
{
    const U64& pawns = BitBoards[PW];
    U64 pawnMoves;
    // Diag left
    pawnMoves = (pawns << 7) & ~FILE_H & ~RANK_8 & (BlackPieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, 7, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, 7, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_LEFT);
    // Diag right
    pawnMoves = (pawns << 9) & ~FILE_A & ~RANK_8 & (BlackPieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, 9, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, 9, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_RIGHT);
    // Forward one
    pawnMoves = (pawns << 8) & ~RANK_8 & Empty;
    AddMovesFromBitboardAbsolute(nonCaptures, pawnMoves, 8, 0);
    // Forward two
    pawnMoves = (((pawns << 8) & Empty) << 8) & WHITE_SIDE & Empty;
    AddMovesFromBitboardAbsolute(nonCaptures, pawnMoves, 16, MOVE_TYPE_PAWN_HOPP); // add move type

    // Promote diag left
    pawnMoves = (pawns << 7) & ~FILE_H & RANK_8 & BlackPieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, 7);
    // Promote diag right
    pawnMoves = (pawns << 9) & ~FILE_A & RANK_8 & BlackPieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, 9);
    // Promote forward
    pawnMoves = (pawns << 8) & RANK_8 & Empty;
    AddMovesFromBitboardPawnPromote(nonCaptures, pawnMoves, 8);
}

void Board::PawnMovesWhite(std::vector<MOVE>& captures) const
{
    const U64& pawns = BitBoards[PW];
    U64 pawnMoves;
    // Diag left
    pawnMoves = (pawns << 7) & ~FILE_H & ~RANK_8 & (BlackPieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, 7, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, 7, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_LEFT);
    // Diag right
    pawnMoves = (pawns << 9) & ~FILE_A & ~RANK_8 & (BlackPieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, 9, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, 9, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_RIGHT);

    // Promote diag left
    pawnMoves = (pawns << 7) & ~FILE_H & RANK_8 & BlackPieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, 7);
    // Promote diag right
    pawnMoves = (pawns << 9) & ~FILE_A & RANK_8 & BlackPieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, 9);
}

void Board::PawnMovesBlack(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const
{
    const U64& pawns = BitBoards[PB];
    U64 pawnMoves;
    // Diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & ~RANK_1 & (WhitePieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, -7, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, -7, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_LEFT);
    // Diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & ~RANK_1 & (WhitePieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, -9, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, -9, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_RIGHT);
    // Forward one
    pawnMoves = (pawns >> 8) & ~RANK_1 & Empty;
    AddMovesFromBitboardAbsolute(nonCaptures, pawnMoves, -8, 0);
    // Forward two
    pawnMoves = (((pawns >> 8) & Empty) >> 8) & BLACK_SIDE & Empty;
    AddMovesFromBitboardAbsolute(nonCaptures, pawnMoves, -16, MOVE_TYPE_PAWN_HOPP); // add move type

    // Promote diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & RANK_1 & WhitePieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, -7);
    // Promote diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & RANK_1 & WhitePieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, -9);
    // Promote forward
    pawnMoves = (pawns >> 8) & RANK_1 & Empty;
    AddMovesFromBitboardPawnPromote(nonCaptures, pawnMoves, -8);
}

void Board::PawnMovesBlack(std::vector<MOVE>& captures) const
{
    const U64& pawns = BitBoards[PB];
    U64 pawnMoves;
    // Diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & ~RANK_1 & (WhitePieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, -7, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, -7, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_LEFT);
    // Diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & ~RANK_1 & (WhitePieces | EnpassantTarget);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & ~EnpassantTarget, -9, 0);
    AddMovesFromBitboardAbsolute(captures, pawnMoves & EnpassantTarget, -9, MOVE_TYPE_ENPASSANT | MOVE_INFO_ENPASSANT_RIGHT);

    // Promote diag left
    pawnMoves = (pawns >> 7) & ~FILE_A & RANK_1 & WhitePieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, -7);
    // Promote diag right
    pawnMoves = (pawns >> 9) & ~FILE_H & RANK_1 & WhitePieces;
    AddMovesFromBitboardPawnPromote(captures, pawnMoves, -9);
}

void Board::AddMovesFromBitboard(std::vector<MOVE>& moves, U64 destinations, int position, int moveData)
{
    int i = 0;
    while (destinations)
    {
        i = trailingZeros(destinations);
        int move = i << 8 | position;
        moves.push_back(MOVE(0, move | moveData)); // combine move with moveData
        destinations ^= 1ULL << i; // unset this bit
    }
}

void Board::AddMovesFromBitboardPawnPromote(std::vector<MOVE>& moves, U64 movedBoard, int offset)
{
    int i = 0;
    while (movedBoard)
    {
        i = trailingZeros(movedBoard);
        int move = i << 8 | (i - offset);
        moves.push_back(MOVE(0, move | MOVE_TYPE_PROMOTE | MOVE_INFO_PROMOTE_QUEEN));
        moves.push_back(MOVE(0, move | MOVE_TYPE_PROMOTE | MOVE_INFO_PROMOTE_ROOK));
        moves.push_back(MOVE(0, move | MOVE_TYPE_PROMOTE | MOVE_INFO_PROMOTE_HORSEY));
        moves.push_back(MOVE(0, move | MOVE_TYPE_PROMOTE | MOVE_INFO_PROMOTE_BISHOP));
        movedBoard ^= 1ULL << i; // unset this bit
    }
}

void Board::AddMovesFromBitboardAbsolute(std::vector<MOVE>& moves, U64 movedBoard, int offset, int moveData)
{
    int i = 0;
    while (movedBoard)
    {
        i = trailingZeros(movedBoard);
        int move = i << 8 | (i - offset);
        moves.push_back(MOVE(0, move | moveData)); // combine move with moveData
        movedBoard ^= 1ULL << i; // unset this bit
    }
}

#undef U64