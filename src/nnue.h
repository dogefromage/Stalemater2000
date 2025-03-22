#pragma once
#include <cstdint>
#include "labels.h"
#include <string>

// partially from: https://www.chessprogramming.org/NNUE

const int INPUT_SIZE = 768;
const int HL_SIZE = 1024;
const int NUM_BUCKETS = 8;

extern const float nnue_accumulator_weights[INPUT_SIZE][HL_SIZE];
extern const float nnue_accumulator_biases[HL_SIZE];
extern const float nnue_output_weights[NUM_BUCKETS][2 * HL_SIZE];
extern const float nnue_output_bias[NUM_BUCKETS];

class Accumulator {
public:
    float white_acc[HL_SIZE];
    float black_acc[HL_SIZE];

    Accumulator();
    
    void add(int bb, int square);
    void remove(int bb, int square);
    int32_t forward(Side side, U64 occupied);
};

void init_nnue(std::string weights_path);