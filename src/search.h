#pragma once

#include "external/chess.hpp"
#include "tt.h"
#include "timeman.h"
#include "nnue.h"
#include <atomic>
#include <cstring>
#include <thread>
#include <algorithm>

using namespace chess;

#define MAX_PLY 125
#define BENCH_DEPTH 9

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

void fillLmr();

struct ThreadInfo {
	ThreadType type;
	TTable &TT;
	std::atomic<bool> &abort;
	Board board;
	Accumulator accumulator;
	std::atomic<uint64_t> nodes;
	Move bestMove;
	int minNmpPly;
	int rootDepth;

	std::array<std::array<std::array<int, 64>, 64>, 2> history;
	//uint64_t ttHits;

	ThreadInfo(ThreadType type, TTable &TT, std::atomic<bool> &abort) : type(type), TT(TT), abort(abort) {
		abort.store(false, std::memory_order_relaxed);
		this->board = Board();
		std::memset(&history, 0, sizeof(history));
		nodes = 0;
		bestMove = Move::NO_MOVE;
		minNmpPly = 0;
		rootDepth = 0;
		//ttHits = 0;
	}
	ThreadInfo(const ThreadInfo &other) : type(other.type), TT(other.TT), abort(other.abort), history(other.history), 
											bestMove(other.bestMove), minNmpPly(other.minNmpPly), rootDepth(other.rootDepth) {
		this->board = other.board;
		nodes.store(other.nodes.load(std::memory_order_relaxed), std::memory_order_relaxed);
	}
	void updateHistory(Color c, Move m, int bonus){
		int clamped = std::clamp(bonus, -16384, 16384);
		history[(int)c][m.from().index()][m.to().index()] += clamped - history[(int)c][m.from().index()][m.to().index()] * std::abs(clamped) / 16384;
	}
	int getHistory(Color c, Move m){
		return history[(int)c][m.from().index()][m.to().index()];
	}
	void reset(){
		nodes.store(0, std::memory_order_relaxed);
		bestMove = Move::NO_MOVE;
		for (auto &i : history)
			for (auto &j : i)
				j.fill(0);
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
    chess::Move killer;
    int    staticEval;
    int historyScore;
};

struct Limit {
	TimeLimit timer;
	int64_t depth;
	int64_t ctime;
	int64_t movetime;
	int64_t maxnodes;
	int64_t softnodes;
	int64_t inc;
	int64_t softtime;
	bool enableClock;
	Color color;

	Limit(){
		depth = 0;
		ctime = 0;
		movetime = 0;
		maxnodes = -1;
		softnodes = -1;
		softtime = 0;
		enableClock = true;
		inc = 0;
	}
	Limit(int64_t depth, int64_t ctime, int64_t movetime, Color color) : depth(depth), ctime(ctime), movetime(movetime), color(color) {
		
	}
	// I will eventually fix this ugly code
	void start(){
		enableClock = movetime != 0 || ctime != 0;
		if (depth == 0)
			depth = MAX_PLY - 5;
		if (enableClock)
			softtime = 0;
		if (ctime != 0){
			// Calculate movetime
			// this was like ~34 lol
			movetime = ctime / (inc <= 0 ? 30 : 20) + inc / 2;
			softtime = movetime * 0.63;
			movetime -= 15;
		}
		timer.start();
	}
	bool outOfNodes(int64_t cnt){
		return maxnodes != -1 && cnt > maxnodes;
	}
	bool softNodes(int64_t cnt){
		return softnodes != -1 && cnt > softnodes;
	}
	bool outOfTime(){
		return (enableClock && static_cast<int64_t>(timer.elapsed()) >= movetime);
	}
	bool outOfTimeSoft(){
		if (!enableClock || softtime == 0)
			return false;
		return (static_cast<int64_t>(timer.elapsed()) >= softtime);
	}
};
//int search(Board &board, int depth, int ply, int alpha, int beta, Stack *ss, ThreadInfo &thread);
//int iterativeDeepening(Board board, ThreadInfo &threadInfo, Searcher *searcher);
int iterativeDeepening(Board &board, ThreadInfo &threadInfo, Limit limit, Searcher *searcher);

void bench();
} 