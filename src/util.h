#pragma once

#include "external/chess.hpp"
#include "nnue.h"
#include <bit>
#include <vector>
#include <sstream>
#include <cassert>
#include <cstring>

using namespace chess;


// Accumulator wrapper
void MakeMove(Board &board, Accumulator &acc, Move &move);
void UnmakeMove(Board &board, Accumulator &acc, Move &move);