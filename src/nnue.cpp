#include "nnue.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include <cassert>
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

void Accumulator::init() {
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

int32_t Accumulator::forward(Side side, U64 occupied) const {
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

// assumes ply is in bounds
void AccumulatorStack::stackUp(int ply) {
    AccumulatorStackNode& node = stack[ply];
    if (!node.dirty) {
        return;
    }
    stackUp(ply - 1);
    AccumulatorStackNode& parent = stack[ply - 1];
    assert(!parent.dirty);

    // copy (TODO check if there is other option)
    node.acc = parent.acc;

    assert(node.recorder.numEdits);  // assume moves always change something on the board
    // apply recorded edits
    for (int i = 0; i < node.recorder.numEdits; i++) {
        BoardEdit& edit = node.recorder.edits[i];
        if (edit.type == BoardEditType::Add) {
            node.acc.add(edit.bb, edit.square);
        } else {
            node.acc.remove(edit.bb, edit.square);
        }
    }

    node.recorder.clear();
    node.dirty = false;
}

void AccumulatorStack::init(const Board& board) {
    AccumulatorStackNode& root = stack[0];
    root.dirty = false;
    root.recorder.clear();

    root.acc.init();

    for (int piece = 0; piece < 12; piece++) {
        U64 bb = board.getBoard((BitBoards)piece);
        while (bb) {
            int square = trailingZeros(bb);
            bb ^= 1ULL << square;
            root.acc.add(piece, square);
        }
    }
}

int32_t AccumulatorStack::forward(int ply, Side side, U64 occupied) {
    assert(ply < ACCUMULATOR_MAX_DEPTH);
    stackUp(ply);
    AccumulatorStackNode& node = stack[ply];
    return node.acc.forward(side, occupied);
}

BoardEditRecorder* AccumulatorStack::getRecorder(int ply) {
    assert(ply < ACCUMULATOR_MAX_DEPTH);
    AccumulatorStackNode& node = stack[ply];
    node.recorder.clear();
    return &node.recorder;
}

void AccumulatorStack::markDirty(int ply) {
    assert(1 <= ply);
    assert(ply < ACCUMULATOR_MAX_DEPTH);
    AccumulatorStackNode& node = stack[ply];
    node.dirty = true;
}
