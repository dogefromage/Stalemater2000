#include "nnue.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>

#include "bitmath.h"

// const int QA = 255;
// const int QB = 128;

int get_input_index(bool flipped, int bb, int square) {
    if (flipped) {
        // switch colors
        bb = (bb + 6) % 12;
        // flip vertically
        square ^= 0b111000;
    }
    return 64 * bb + square;
}

float SCReLU(float x) {
    if (x < 0) {
        return 0;
    }
    if (x > 1) {
        x = 1;
    }
    return x * x;
}

Accumulator::Accumulator() {
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] = nnue_accumulator_biases[i];
        black_acc[i] = nnue_accumulator_biases[i];
    }
}

void Accumulator::add(int bb, int square) {
    int white_input_index = get_input_index(false, bb, square);
    int black_input_index = get_input_index(true, bb, square);
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] += nnue_accumulator_weights[white_input_index][i];
        black_acc[i] += nnue_accumulator_weights[black_input_index][i];
    }
}

void Accumulator::remove(int bb, int square) {
    int white_input_index = get_input_index(false, bb, square);
    int black_input_index = get_input_index(true, bb, square);
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] -= nnue_accumulator_weights[white_input_index][i];
        black_acc[i] -= nnue_accumulator_weights[black_input_index][i];
    }
}

int32_t Accumulator::forward(Side side, U64 occupied) {
    // CONCATENATE AND OUTPUT
    int white_start = 0;
    int black_start = HL_SIZE;
    if (side == Side::Black) {
        // concatenate in reverse
        white_start = HL_SIZE;
        black_start = 0;
    }

    int num_pieces = countBits(occupied);
    int output_bucket = int((num_pieces - 2) / (31.0f / (float)NUM_BUCKETS));
    output_bucket = std::min(output_bucket, NUM_BUCKETS - 1);

    float output = 0;
    for (int i = 0; i < HL_SIZE; i++) {
        output += SCReLU(white_acc[i]) * nnue_output_weights[output_bucket][white_start + i];
        output += SCReLU(black_acc[i]) * nnue_output_weights[output_bucket][black_start + i];
    }
    
    return (int)output;
}
