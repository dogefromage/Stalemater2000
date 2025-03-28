#pragma once

typedef unsigned long long U64;

// https://www.chessprogramming.org/Encoding_Moves#MoveIndex
const int MAXIMUM_POSSIBLE_MOVES = 218;
const int ACCUMULATOR_MAX_DEPTH = 64;
const int MAX_BOARD_EDITS_PER_MOVE = 8;

enum class BitBoards {
	PW, RW, NW, BW, QW, KW, PB, RB, NB, BB, QB, KB,
};

enum class MovePromotions {
	None, Q, R, N, B,
};

enum class MoveTypes {
	Normal,
	PawnDouble,
	EnpasKing,
	EnpasQueen,
	Promote,
	CastleWhiteKing,
	CastleWhiteQueen,
	CastleBlackKing,
	CastleBlackQueen,
};

enum class CaptureType {
	NonCapture,
	Capture,
};

enum class CastlingTypes {
	WhiteKing, WhiteQueen, BlackKing, BlackQueen,
};

enum class Side {
	White, Black,
};

enum class CheckFlags {
	WhiteInCheck = 1,
	BlackInCheck = 2,
};

// MASKS
constexpr U64 FILE_A = 0x101010101010101ULL;
constexpr U64 FILE_AB = 0x303030303030303ULL;
constexpr U64 FILE_GH = 0xc0c0c0c0c0c0c0c0ULL;
constexpr U64 FILE_H = 0x8080808080808080ULL;
constexpr U64 RANK_1 = 0xffULL;
constexpr U64 RANK_8 = 0xff00000000000000ULL;
constexpr U64 FILE_MASKS[] = {
	0x101010101010101ULL,0x202020202020202ULL,0x404040404040404ULL,0x808080808080808ULL,
	0x1010101010101010ULL,0x2020202020202020ULL,0x4040404040404040ULL,0x8080808080808080ULL,
};
constexpr U64 RANK_MASKS[] = {
	0xffULL,0xff00ULL,0xff0000ULL,0xff000000ULL,0xff00000000ULL,
	0xff0000000000ULL,0xff000000000000ULL,0xff00000000000000ULL
};
constexpr U64 DIAG_MASK[] = {
	0x1ULL, 0x102ULL, 0x10204ULL, 0x1020408ULL, 0x102040810ULL, 0x10204081020ULL, 0x1020408102040ULL,
	0x102040810204080ULL, 0x204081020408000ULL, 0x408102040800000ULL, 0x810204080000000ULL,
	0x1020408000000000ULL, 0x2040800000000000ULL, 0x4080000000000000ULL, 0x8000000000000000ULL
};
constexpr U64 ANTIDIAG_MASK[] = {
	0x80ULL, 0x8040ULL, 0x804020ULL, 0x80402010ULL, 0x8040201008ULL, 0x804020100804ULL, 0x80402010080402ULL,
	0x8040201008040201ULL, 0x4020100804020100ULL, 0x2010080402010000ULL, 0x1008040201000000ULL,
	0x804020100000000ULL, 0x402010000000000ULL, 0x201000000000000ULL, 0x100000000000000ULL
};

constexpr U64 RING_MASK[] = {
	0xFF818181818181FFULL, 0x7E424242427E00ULL, 0x3C24243C0000ULL, 0x1818000000ULL
};
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

constexpr U64 SPAN_HORSE = 0xA1100110AULL;
constexpr U64 SPAN_HORSE_OFFSET = 18;
constexpr U64 SPAN_KING = 0x70707ULL;
constexpr U64 SPAN_KING_OFFSET = 9;

constexpr U64 WHITE_SIDE = 0xFFFFFFFFULL;
constexpr U64 BLACK_SIDE = 0xFFFFFFFF00000000ULL;