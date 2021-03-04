#pragma once

// offset to get right values
constexpr int ZOBRISTTABLE_PIECES = 0;
constexpr int ZOBRISTTABLE_ENPASSANT = 768;
constexpr int ZOBRISTTABLE_BLACK_MOVE = 832;
constexpr int ZOBRISTTABLE_CASTLING = 833;

//                               pieces,   enpas, blackMove, castling
constexpr int zobristTableSize = 64 * 12 + 64 +   1 +        4; // 837 total

extern unsigned long long ZobristValues[];
void InitZobrist();
