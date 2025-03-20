#pragma once
#include <cstdint>
#include "labels.h"
#include <string>

// partially from: https://www.chessprogramming.org/NNUE

const int INPUT_SIZE = 768;
const int HL_SIZE = 1024;

struct NNUE {
    int16_t accumulator_weights[INPUT_SIZE][HL_SIZE]; // column major
    int16_t accumulator_biases[HL_SIZE];
    int16_t output_weights[2 * HL_SIZE];
    int16_t output_bias;
};

class Accumulator {
public:
    int16_t white_acc[HL_SIZE];
    int16_t black_acc[HL_SIZE];

    Accumulator();
    
    void add(int bb, int square);
    void remove(int bb, int square);
    int32_t forward(Side side);
};

void init_nnue(std::string weights_path);