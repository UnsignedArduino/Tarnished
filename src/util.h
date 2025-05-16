#pragma once

#include "external/chess.hpp"
#include "nnue.h"
#include <bit>
#include <vector>
#include <sstream>
#include <cassert>
#include <cstring>

using namespace chess;

enum DirIndex {
	NORTH = 0,
	SOUTH = 1,
	EAST = 2,
	WEST = 3,
	NORTH_EAST = 4,
	NORTH_WEST = 5,
	SOUTH_EAST = 6,
	SOUTH_WEST = 7
};

struct StateInfo {
	Bitboard pinners[2];
	Bitboard kingBlockers[2];
	StateInfo(){
		pinners[0] = Bitboard(0); pinners[1] = Bitboard(0);
		kingBlockers[0] = Bitboard(0); kingBlockers[1] = Bitboard(0);
	}
};
// Values taken from SF
constexpr int PawnValue   = 208;
constexpr int KnightValue = 781;
constexpr int BishopValue = 825;
constexpr int RookValue   = 1276;
constexpr int QueenValue  = 2538;

constexpr int PieceValue[] = {PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0};


// Accumulator wrapper
void MakeMove(Board &board, Accumulator &acc, Move &move);
void UnmakeMove(Board &board, Accumulator &acc, Move &move);
// SEE stuff
void initLookups();
int oppDir(int dir);
Bitboard attackersTo(Board &board, Square s, Bitboard occ);
void pinnersBlockers(Board &board, Color c, StateInfo sti);
bool SEE(Board &board, Move &move, int margin);