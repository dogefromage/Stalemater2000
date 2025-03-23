#pragma once
#include <cstdint>
#include <string>

#include "board.h"
#include "labels.h"

// partially from: https://www.chessprogramming.org/NNUE

const int INPUT_SIZE = 768;
const int HL_SIZE = 1024;
const int NUM_BUCKETS = 8;

/**
 * Will be linked to asm file of weights created by python script
 */
extern const float nnue_accumulator_weights[INPUT_SIZE][HL_SIZE];
extern const float nnue_accumulator_biases[HL_SIZE];
extern const float nnue_output_weights[NUM_BUCKETS][2 * HL_SIZE];
extern const float nnue_output_bias[NUM_BUCKETS];

class Accumulator {
   public:
    void init();
    void add(int bb, int square);
    void remove(int bb, int square);
    int32_t forward(Side side, U64 occupied) const;

   private:
    float white_acc[HL_SIZE];
    float black_acc[HL_SIZE];
};

struct AccumulatorStackNode {
    Accumulator acc;
    BoardEditRecorder recorder;  // holds edits regarding this board relative to the above one
    bool dirty;                  // if true, accumulator is valid, otherwise apply recorded edits to parent board
};

class AccumulatorStack {
   public:
    void init(const Board& board);
    int32_t forward(int ply, Side side, U64 occupied);
    BoardEditRecorder* getRecorder(int targetPly);
    void markDirty(int ply);

   private:
    AccumulatorStackNode stack[ACCUMULATOR_MAX_DEPTH];

    void stackUp(int ply);
};