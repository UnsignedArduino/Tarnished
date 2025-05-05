#pragma once

#include "external/chess.hpp"
#include "tt.h"
#include "timeman.h"
#include <atomic>
#include <cstring>
#include <thread>
#include <algorithm>

using namespace chess;

#define MAX_PLY 125

const int MVVLVA[7][7] = {
    {15, 14, 13, 12, 11, 10, 0}, // Victim P, attacker P, N, B, R, Q, K, None
    {25, 24, 23, 22, 21, 20, 0}, // Victim N, attacker P, N, B, R, Q, K, None
    {35, 34, 33, 32, 31, 30, 0}, // Victim B, attacker P, N, B, R, Q, K, None
    {45, 44, 43, 42, 41, 40, 0}, // Victim R, attacker P, N, B, R, Q, K, None
    {55, 54, 53, 52, 51, 50, 0}, // Victim Q, attacker P, N, B, R, Q, K, None
    {0,  0,  0,  0,  0,  0,  0}  // Victim None, attacker P, N, B, R, Q, K, None
};

enum ThreadType {
    MAIN      = 1,
    SECONDARY = 0
};

struct Searcher;

namespace Search {

struct ThreadInfo {
	ThreadType type;
	TTable &TT;
	std::atomic<bool> &abort;
	Board board;
	std::atomic<uint64_t> nodes;
	uint64_t ttHits;

	ThreadInfo(ThreadType type, TTable &TT, std::atomic<bool> &abort) : type(type), TT(TT), abort(abort) {
		abort.store(false, std::memory_order_relaxed);
		this->board = Board();
		nodes = 0;
		ttHits = 0;
	}
	ThreadInfo(const ThreadInfo &other) : type(other.type), TT(other.TT), abort(other.abort), ttHits(other.ttHits) {
		this->board = other.board;
		nodes.store(other.nodes.load(std::memory_order_relaxed), std::memory_order_relaxed);
	}
	void reset(){
		nodes.store(0, std::memory_order_relaxed);
	}
};

struct PVList {
	std::array<chess::Move, MAX_PLY> moves;
	uint32_t length;
	PVList(){
		length = 0;
	}
	void update(chess::Move move, const PVList &who){
		moves[0] = move;
		std::copy(who.moves.begin(), who.moves.begin() + who.length, moves.begin() + 1);
		length = who.length + 1;
	}
	auto &operator=(const PVList &other){
		std::copy(other.moves.begin(), other.moves.begin() + other.length, moves.begin());
		this->length = other.length;
		return *this;
	}
};

struct Stack {
    PVList pv;
    chess::Move   excluded;
    int    staticEval;
};

struct Limit {
	TimeLimit timer;
	int64_t depth;
	int64_t ctime;
	int64_t movetime;
	Color color;

	Limit(){
		depth = 0;
		ctime = 0;
		movetime = 0;
	}
	Limit(int64_t depth, int64_t ctime, int64_t movetime, Color color) : depth(depth), ctime(ctime), movetime(movetime), color(color) {
		
	}
	void start(){
		if (movetime == 0){
			if (depth != 0){
				//depth = 16;
				movetime = 32000;
			}
			else {
				movetime = ctime;
				movetime /= 20;	
			}
		}
		if (depth == 0)
			depth = 32;
		timer.start();
	}
	bool outOfTime(){
		return static_cast<int64_t>(timer.elapsed()) >= movetime - 25;
	}
};
//int search(Board &board, int depth, int ply, int alpha, int beta, Stack *ss, ThreadInfo &thread);
//int iterativeDeepening(Board board, ThreadInfo &threadInfo, Searcher *searcher);
 
} 