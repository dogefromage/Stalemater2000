#pragma once
#include <vector>
#include <string>

#include "BitMath.h"
#include "Zobrist.h"

#define U64 unsigned long long

#define MOVE std::pair<short, int>

// BITBOARD INDEXES
constexpr int PW = 0;
constexpr int RW = 1;
constexpr int NW = 2;
constexpr int BW = 3;
constexpr int QW = 4;
constexpr int KW = 5;
constexpr int PB = 6;
constexpr int RB = 7;
constexpr int NB = 8;
constexpr int BB = 9;
constexpr int QB = 10;
constexpr int KB = 11;
// MOVE RIGHT
constexpr int WHITE_TO_MOVE = 0;
constexpr int BLACK_TO_MOVE = 1;
// CASTLE RIGHTS
constexpr int CASTLE_KW = 1;
constexpr int CASTLE_QW = 2;
constexpr int CASTLE_KB = 4;
constexpr int CASTLE_QB = 8;
constexpr int WHITE_HAS_CASTLED = 1;
constexpr int BLACK_HAS_CASTLED = 2;
// FILES
constexpr U64 FILE_A = 0x101010101010101ULL;
constexpr U64 FILE_AB = 0x303030303030303ULL;
constexpr U64 FILE_GH = 0xc0c0c0c0c0c0c0c0ULL;
constexpr U64 FILE_H = 0x8080808080808080ULL;
// RANKS
constexpr U64 RANK_1 = 0xffULL;
constexpr U64 RANK_8 = 0xff00000000000000ULL;
// MASKS
constexpr U64 FILE_MASKS[] =
{
    0x101010101010101ULL,0x202020202020202ULL,0x404040404040404ULL,0x808080808080808ULL,
    0x1010101010101010ULL,0x2020202020202020ULL,0x4040404040404040ULL,0x8080808080808080ULL,
};
constexpr U64 RANK_MASKS[] =
{
    0xffULL,0xff00ULL,0xff0000ULL,0xff000000ULL,0xff00000000ULL,
    0xff0000000000ULL,0xff000000000000ULL,0xff00000000000000ULL
};
constexpr U64 DIAG_MASK[] =
{
    0x1ULL, 0x102ULL, 0x10204ULL, 0x1020408ULL, 0x102040810ULL, 0x10204081020ULL, 0x1020408102040ULL,
    0x102040810204080ULL, 0x204081020408000ULL, 0x408102040800000ULL, 0x810204080000000ULL,
    0x1020408000000000ULL, 0x2040800000000000ULL, 0x4080000000000000ULL, 0x8000000000000000ULL
};
constexpr U64 ANTIDIAG_MASK[] =
{
    0x80ULL, 0x8040ULL, 0x804020ULL, 0x80402010ULL, 0x8040201008ULL, 0x804020100804ULL, 0x80402010080402ULL,
    0x8040201008040201ULL, 0x4020100804020100ULL, 0x2010080402010000ULL, 0x1008040201000000ULL,
    0x804020100000000ULL, 0x402010000000000ULL, 0x201000000000000ULL, 0x100000000000000ULL
};
// CASTLEMASKS
constexpr U64 CASTLE_MASK_W_K_PATH = 0x70ULL;
constexpr U64 CASTLE_MASK_W_K_GAP = 0x60ULL;
constexpr U64 CASTLE_MASK_W_Q_PATH = 0x1CULL;
constexpr U64 CASTLE_MASK_W_Q_GAP = 0xEULL;
constexpr U64 CASTLE_MASK_B_K_PATH = 0x7000000000000000ULL;
constexpr U64 CASTLE_MASK_B_K_GAP = 0x6000000000000000ULL;
constexpr U64 CASTLE_MASK_B_Q_PATH = 0x1C00000000000000ULL;
constexpr U64 CASTLE_MASK_B_Q_GAP = 0xE00000000000000ULL;
constexpr U64 CASTLE_MASK_PIECES_WHITE_KING = 0x90ULL;
constexpr U64 CASTLE_MASK_PIECES_WHITE_QUEEN = 0x11ULL;
constexpr U64 CASTLE_MASK_PIECES_BLACK_KING = 0x9000000000000000ULL;
constexpr U64 CASTLE_MASK_PIECES_BLACK_QUEEN = 0x1100000000000000ULL;

// SPANS
constexpr U64 SPAN_HORSE = 0xA1100110AULL;
constexpr U64 SPAN_HORSE_OFFSET = 18;
constexpr U64 SPAN_KING = 0x70707ULL;
constexpr U64 SPAN_KING_OFFSET = 9;
// REGIONS
constexpr U64 WHITE_SIDE = 0xFFFFFFFFULL;
constexpr U64 BLACK_SIDE = 0xFFFFFFFF00000000ULL;
/**
 * MOVES ARE REPRESENTED USING int32_t
 * first 8 bits: from
 * second 8 bits: to
 * rest: types/info
 */
constexpr int MOVE_TYPE_NONE = 0x0;
constexpr int MOVE_TYPE_PAWN_HOPP = 0x10000;
constexpr int MOVE_TYPE_ENPASSANT = 0x20000;
constexpr int MOVE_TYPE_PROMOTE = 0x40000;
constexpr int MOVE_TYPE_CASTLE = 0x80000;
constexpr int MOVE_INFO_ENPASSANT_LEFT = 0x1000000;
constexpr int MOVE_INFO_ENPASSANT_RIGHT = 0x2000000;
constexpr int MOVE_INFO_PROMOTE_QUEEN = 0x1000000;
constexpr int MOVE_INFO_PROMOTE_HORSEY = 0x2000000;
constexpr int MOVE_INFO_PROMOTE_BISHOP = 0x4000000;
constexpr int MOVE_INFO_PROMOTE_ROOK = 0x8000000;
constexpr int MOVE_INFO_CASTLE_KINGSIDE = 0x1000000;
constexpr int MOVE_INFO_CASTLE_QUEENSIDE = 0x2000000;
// POSITION STATUS
constexpr int POSITION_ILLEGAL = 0;
constexpr int POSITION_LEGAL = 1;
constexpr int POSITION_WHITE_CHECKED = 2;
constexpr int POSITION_BLACK_CHECKED = 4;
// CHECK
constexpr int CHECK_WHITE = 1;
constexpr int CHECK_BLACK = 2;

class Board
{
public:
    // Properties of board (must be hashed):
    U64 BitBoards[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    U64 EnpassantTarget = 0;
    char SideToMove = 0;
    char Castling = 0;
    char HasCastled = 0;
    // Good to know:
    U64 WhitePieces = 0, BlackPieces = 0, NotWhitePieces = 0, NotBlackPieces = 0, Empty = 0, Occupied = 0, UnsafeForWhite = 0, UnsafeForBlack = 0;
    U64 Zobrist = 0;
    char Checks = 0;

    Board() {}

    Board(const U64 bitBoards[], const char sideToMove, const char castling, const char hasCastled, const U64 enpassantTarget, const U64 zobrist);

    static Board Default();

    /**
     * Make move and return new updated board (the move won't always be valid because it's a pseudo move)
     */
    Board Move(const int move) const;

    void ExecuteMove(int bb, int from, int to);

    void ForbidCastling(int castlingType);

    void SetEnpassantTarget(U64 newTarget);

    int GetBoardStatus() const;

    /**
     * GENERATES PSEUDO MOVES (SOME MIGHT BE ILLEGAL, LIKE DISCOVERY ATTACKS ON KING).
     * IF CURRENT POSITION IS ILLEGAL, 0 IS RETURNED
     */
    void GeneratePseudoMoves(std::vector<MOVE>& moveList) const;

    /**
     * GENERATES LEGAL MOVES (SLOW!)
     */
    void GenerateLegalMoves(std::vector<MOVE>& moveList) const;

    /**
    * only pseudo-captures
    */
    void GenerateCaptures(std::vector<MOVE>& moveList) const;

    static std::string MoveToText(int m, bool onlyMove);

    static std::string IndexToText(int index);

    static int TextToIndex(const std::string& text);

    static Board FromFEN(const std::string& fenInput);

    std::string ToFEN() const;

    void Print() const;

    void PrintAllBitBoards() const;

    static void PrintBitBoard(const U64& board);

    // Generates zobrist from scratch (slow)
    void GenerateZobrist();

private:
    void Init();

    void FindUnsafeForWhite();

    void FindUnsafeForBlack();

    U64 HAndVMoves(int index) const;

    U64 DandAntiDMoves(int index) const;

    /**
    *  Move generators; some have a only-capture version for quiescence search
    */
    void CastlesWhite(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const;
    void CastlesBlack(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const;

    void MoveSlidingPiece(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, U64 bitboard, bool paral, bool diag, bool white) const;
    void MoveSlidingPiece(std::vector<MOVE>& captures, U64 bitboard, bool paral, bool diag, bool white) const;

    void KingMoves(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, bool isWhite) const;
    void KingMoves(std::vector<MOVE>& captures, bool isWhite) const;

    void KnightMoves(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures, bool isWhite) const;
    void KnightMoves(std::vector<MOVE>& captures, bool isWhite) const;

    void PawnMovesWhite(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const;
    void PawnMovesWhite(std::vector<MOVE>& captures) const;
    void PawnMovesBlack(std::vector<MOVE>& captures, std::vector<MOVE>& nonCaptures) const;
    void PawnMovesBlack(std::vector<MOVE>& captures) const;

    static void AddMovesFromBitboard(std::vector<MOVE>& moves, U64 destinations, int position, int moveData);

    static void AddMovesFromBitboardPawnPromote(std::vector<MOVE>& moves, U64 movedBoard, int offset);

    static void AddMovesFromBitboardAbsolute(std::vector<MOVE>& moves, U64 movedBoard, int offset, int moveData);
};