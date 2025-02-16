#pragma once
#include <cstdint>

#include "board.h"

// partially from: https://www.chessprogramming.org/NNUE

const int INPUT_SIZE = 768;
const int HL_SIZE = 1024;

const int SCALE = 1000;

typedef int32_t nnue_int_t;

struct NNUE {
    nnue_int_t accumulator_weights[INPUT_SIZE][HL_SIZE]; // row major
    nnue_int_t accumulator_biases[HL_SIZE];
    nnue_int_t output_weights[2 * HL_SIZE];
    nnue_int_t output_bias;
};

// struct Accumulator {
//     int16_t values[HL_SIZE];
// };

// struct AccumulatorPair {
//     Accumulator white;
//     Accumulator black;
// };

nnue_int_t evaluate_board_nnue(const Board& board);

void init_nnue();