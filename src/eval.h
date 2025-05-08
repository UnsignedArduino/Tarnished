#pragma once
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "external/chess.hpp"

using namespace chess;

#define MATE 31000
#define INFINITE MATE + 1
#define NO_SCORE MATE + 2

#define MakeScore(mg, eg) ((int32_t)((uint32_t)(eg) << 16) + (mg))
#define S(mg, eg) MakeScore((mg), (eg))
#define MgScore(s) ((int16_t)((uint16_t)((uint32_t)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((uint32_t)((s) + 0x8000) >> 16)))

typedef int32_t Score;
