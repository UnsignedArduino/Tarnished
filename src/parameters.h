#pragma once


// NNUE Parameters

constexpr int16_t HL_N = 128;
constexpr int16_t QA = 255;
constexpr int16_t QB = 64;
constexpr int16_t NNUE_SCALE = 400;
constexpr int OUTPUT_BUCKETS = 8; 

// Search Parameters
constexpr int RFP_MARGIN = 80;
constexpr int RFP_MAX_DEPTH = 6;

constexpr int NMP_BASE_REDUCTION = 3;
constexpr int NMP_REDUCTION_SCALE = 4;
constexpr int NMP_EVAL_SCALE = 200;

constexpr int LMR_MIN_DEPTH = 2;

constexpr int BUTTERFLY_MULTIPLIER = 20;

constexpr int IIR_MIN_DEPTH = 6;

constexpr int LMP_MIN_MOVES_BASE = 2;

constexpr int MIN_ASP_WINDOW_DEPTH = 6;
constexpr int INITIAL_ASP_WINDOW = 40; 
constexpr int ASP_WIDENING_FACTOR = 3;