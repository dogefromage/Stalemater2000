#include "nnue.h"

#include <arpa/inet.h>

#include <cstring>

#include "bitmath.h"

// basic implementation without any optimization

NNUE nnue;

int get_input_index(bool flipped, int bb, int square) {
    if (flipped) {
        // switch colors
        bb = (bb + 6) % 12;
        // flip vertically
        square ^= 0b111000;
    }
    return 64 * bb + square;
}

// clamps x between 0 and 1 but values are scaled by SCALE
nnue_int_t SCReLU(nnue_int_t x) {
    if (x < 0) {
        return 0;
    }
    if (x > SCALE) {
        x = SCALE;
    }
    return x * x / SCALE;
}

nnue_int_t evaluate_board_nnue(const Board& board) {
    // INPUT LAYER
    nnue_int_t white_input[INPUT_SIZE], black_input[INPUT_SIZE];
    memset(white_input, 0, sizeof(white_input));
    memset(black_input, 0, sizeof(black_input));
    for (int b = 0; b < 12; b++) {
        U64 bb = board.getBoard((BitBoards)b);
        while (bb) {
            int i = trailingZeros(bb);
            bb ^= 1ULL << i;
            white_input[get_input_index(false, b, i)] = 1;
            black_input[get_input_index(true, b, i)] = 1;
        }
    }

    // ACCUMULATORS
    nnue_int_t white_accumulator[HL_SIZE], black_accumulator[HL_SIZE];
    for (int i = 0; i < HL_SIZE; i++) {
        nnue_int_t acc = 0;
        for (int j = 0; j < INPUT_SIZE; j++) {
            acc += nnue.accumulator_weights[j][i] * white_input[j];
        }
        white_accumulator[i] = acc + nnue.accumulator_biases[i];
    }
    for (int i = 0; i < HL_SIZE; i++) {
        nnue_int_t acc = 0;
        for (int j = 0; j < INPUT_SIZE; j++) {
            acc += nnue.accumulator_weights[j][i] * black_input[j];
        }
        black_accumulator[i] = acc + nnue.accumulator_biases[i];
    }

    // CONCATENATE AND OUTPUT
    int white_start = 0;
    int black_start = HL_SIZE;
    if (board.getSideToMove() == Side::Black) {
        // concatenate in reverse
        white_start = HL_SIZE;
        black_start = 0;
    }

    nnue_int_t output = 0;
    for (int i = 0; i < HL_SIZE; i++) {
        output += nnue.output_weights[white_start + i] * SCReLU(white_accumulator[i]);
        output += nnue.output_weights[black_start + i] * SCReLU(black_accumulator[i]);
    }
    output /= SCALE;
    output += nnue.output_bias;
    output /= SCALE;
    return output;
}

void load_weights(const char* filepath, nnue_int_t* target, size_t count) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        printf("Could not open parameter file %s\n", filepath);
        return;
    }
    for (size_t i = 0; i < count; i++) {
        nnue_int_t param;
        fread(&param, sizeof(nnue_int_t), 1, file);
        param = ntohl(param);
        *target = param;
        target++;
    }
}

void init_nnue() {
    printf("Loading NNUE\n");
    load_weights("weights/accumulation_layer.weight.bin", nnue.accumulator_weights[0], INPUT_SIZE * HL_SIZE);
    load_weights("weights/accumulation_layer.bias.bin", nnue.accumulator_biases, HL_SIZE);
    load_weights("weights/output_layer.weight.bin", nnue.output_weights, 2 * HL_SIZE);
    load_weights("weights/output_layer.bias.bin", &nnue.output_bias, 1);
}
