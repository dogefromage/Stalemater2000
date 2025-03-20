#include "nnue.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>

#include "bitmath.h"

// basic implementation without any optimization

NNUE nnue;

const int QA = 255;
const int QB = 128;

int get_input_index(bool flipped, int bb, int square) {
    if (flipped) {
        // switch colors
        bb = (bb + 6) % 12;
        // flip vertically
        square ^= 0b111000;
    }
    return 64 * bb + square;
}

int32_t SCReLU(int16_t x) {
    if (x < 0) {
        return 0;
    }
    if (x > QA) {
        x = QA;
    }
    return x * x;
}

Accumulator::Accumulator() {
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] = nnue.accumulator_biases[i];
        black_acc[i] = nnue.accumulator_biases[i];
    }
}

void Accumulator::add(int bb, int square) {
    int white_input_index = get_input_index(false, bb, square);
    int black_input_index = get_input_index(true, bb, square);
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] += nnue.accumulator_weights[white_input_index][i];
        black_acc[i] += nnue.accumulator_weights[black_input_index][i];
    }
}

void Accumulator::remove(int bb, int square) {
    int white_input_index = get_input_index(false, bb, square);
    int black_input_index = get_input_index(true, bb, square);
    for (int i = 0; i < HL_SIZE; i++) {
        white_acc[i] -= nnue.accumulator_weights[white_input_index][i];
        black_acc[i] -= nnue.accumulator_weights[black_input_index][i];
    }
}

int32_t Accumulator::forward(Side side) {
    // CONCATENATE AND OUTPUT
    int white_start = 0;
    int black_start = HL_SIZE;
    if (side == Side::Black) {
        // concatenate in reverse
        white_start = HL_SIZE;
        black_start = 0;
    }

    int32_t output = 0;
    for (int i = 0; i < HL_SIZE; i++) {
        output += SCReLU(white_acc[i]) * nnue.output_weights[white_start + i];
        output += SCReLU(black_acc[i]) * nnue.output_weights[black_start + i];
    }
    output /= QA * QA;
    output += nnue.output_bias;
    output /= QB;

    return output;
}

void init_nnue(std::string weights_path) {
    
    FILE* file = fopen(weights_path.c_str(), "r");

    for (int i = 0; i < 4; i++) {
        char layer_name[64];
        int rows, columns;
        if (fscanf(file, "%s %d %d\n", layer_name, &rows, &columns) != 3) {
            printf("Error while reading weights\n");
            exit(1);
        }
        // printf("Loading %s %d %d\n", layer_name, rows, columns);

        int16_t* weights = NULL;
        int16_t scale = 1;

        if (!strcmp(layer_name, "accumulation_layer.weight")) {
            weights = nnue.accumulator_weights[0];
            scale = QA;
        } else if (!strcmp(layer_name, "accumulation_layer.bias")) {
            weights = nnue.accumulator_biases;
            scale = QA;
        } else if (!strcmp(layer_name, "output_layer.weight")) {
            weights = nnue.output_weights;
            scale = QB;
        } else if (!strcmp(layer_name, "output_layer.bias")) {
            weights = &nnue.output_bias;
            scale = QB;
        } else {
            printf("Unknown layer %s\n", layer_name);
            exit(1);
        }

        for (int j = 0; j < columns; j++) {
            for (int i = 0; i < rows; i++) {
                float param;
                if (fscanf(file, "%f\n", &param) != 1) {
                    printf("Expected float\n");
                    exit(1);
                }
                weights[j * rows + i] = (int16_t)(param * scale);
            }
        }
    }

    fclose(file);
}
