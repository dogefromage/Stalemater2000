#pragma once

// offset to get right values
constexpr int ZOBRIST_PIECES = 0;
constexpr int ZOBRIST_ENPASSANT = 768;
constexpr int ZOBRIST_BLACK_MOVE = 832;
constexpr int ZOBRIST_CASTLING = 833;

//                               pieces,   enpas, blackMove, castling
constexpr int zobristTableSize = 64 * 12 + 64 +   1 +        4; // 837 total

extern unsigned long long ZobristValues[];
void InitZobrist();
