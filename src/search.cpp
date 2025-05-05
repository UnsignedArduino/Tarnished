#include "search.h"
#include "searcher.h"
#include "eval.h"
#include "tt.h"

#include <algorithm>
#include <random>

using namespace chess;

// Searcher stuff



namespace Search {
	int16_t MVVLVA(Move &move, ThreadInfo &thread){
		int v = PIECE_VALUES[thread.board.at<PieceType>(move.to())];
		int a = PIECE_VALUES[thread.board.at<PieceType>(move.from())];
		return v * 100 - a;
	}
	int qsearch(int ply, int alpha, const int beta, Stack *ss, ThreadInfo &thread, Limit &limit){
		bool isPV = alpha != beta - 1;
		TTEntry *ttEntry = thread.TT.getEntry(thread.board.hash());
		bool ttHit = ttEntry->zobrist == thread.board.hash();
		if (!isPV && ttHit
			&& (ttEntry->flag == TTFlag::EXACT 
				|| (ttEntry->flag == TTFlag::BETA_CUT && ttEntry->score >= beta)
				|| (ttEntry->flag == TTFlag::FAIL_LOW && ttEntry->score <= alpha))){
			//thread.ttHits ++;
			return ttEntry->score;
		}

		int score = Evaluate(thread.board, thread.board.sideToMove());
		if (ply >= MAX_PLY)
			return score;
		// if (isPV)
		// 	ss->pv.length = 0;

		if (score >= beta)
			return score;
		if (score > alpha)
			alpha = score;

		int bestScore = score;

		Movelist moves;
		movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves, thread.board);

		// Qsearch MVVLVA Sorting
		// for (auto &move : moves){
		// 	int16_t s = MVVLVA(move, thread);
		// 	move.setScore(s);
		// }
		// std::sort(moves.begin(), moves.end(), [](Move a, Move b)
		// 										{
		// 											return a.score() > b.score();
		// 										});
		for (const auto &move : moves){
			if (thread.abort.load(std::memory_order_relaxed))
				return bestScore;
			if (limit.outOfTime()){
				thread.abort.store(true, std::memory_order_relaxed);
				return bestScore;
			}
			thread.board.makeMove(move);
			thread.nodes++;
			score = -qsearch(ply+1, -beta, -alpha, ss+1, thread, limit);
			thread.board.unmakeMove(move);

			if (score > bestScore){
				bestScore = score;
				if (score > alpha)
					alpha = score;
			}
			if (score >= beta){
				break;
			}
		}
		return bestScore;

	}
	int search(int depth, int ply, int alpha, int beta, Stack *ss, ThreadInfo &thread, Limit &limit){
		bool isPV = alpha != beta - 1;
		bool root = ply == 0;
		if (isPV) 
			ss->pv.length = 0;
		if (depth <= 0){
			return qsearch(ply, alpha, beta, ss, thread, limit);
		}

		// Terminal Conditions (and checkmate)
		// Elo difference: 90.8 +/- 24.8
		if (!root){
			if (thread.board.isRepetition(1) || thread.board.isHalfMoveDraw())
				return 0;
		}


		TTEntry *ttEntry = thread.TT.getEntry(thread.board.hash());
		bool ttHit = ttEntry->zobrist == thread.board.hash();
		if (!isPV && ttHit && ttEntry->depth >= depth
			&& (ttEntry->flag == TTFlag::EXACT 
				|| (ttEntry->flag == TTFlag::BETA_CUT && ttEntry->score >= beta)
				|| (ttEntry->flag == TTFlag::FAIL_LOW && ttEntry->score <= alpha))){
			//thread.ttHits++;
			return ttEntry->score;
		}

		int bestScore = -INFINITE;
		int score = bestScore;
		int moveCount = 0;
		bool inCheck = thread.board.inCheck();
		int eval;
		uint8_t ttFlag = TTFlag::FAIL_LOW;
		if (isPV || inCheck)
			goto move_loop;

		// Pruning
		eval = Evaluate(thread.board, thread.board.sideToMove());

		// Reverse Futility Pruning
		// if (depth <= 6 && !root && eval - 80 * depth >= beta && std::abs(beta) < MATE)
		// 	return eval;

	move_loop:
		Move bestMove = Move();
		Movelist moves;

		movegen::legalmoves(moves, thread.board);

		// MVVLVA Sorting
		// Elo difference: 169.6 +/- 35.8
		for (auto &move : moves){
			int16_t s = MVVLVA(move, thread);
			move.setScore(s);
		}
		std::sort(moves.begin(), moves.end(), [](Move a, Move b)
												{
													return a.score() > b.score();
												});
		// if (thread.type != ThreadType::MAIN){
		// 	auto rng = std::default_random_engine {};
		// 	std::shuffle(std::begin(moves), std::end(moves), rng);
		// }
		for (const auto &move : moves){
			if (thread.abort.load(std::memory_order_relaxed))
				return bestScore;
			if (limit.outOfTime()){
				thread.abort.store(true, std::memory_order_relaxed);
				return bestScore;
			}

			thread.board.makeMove<true>(move);
			int newDepth = depth-1;
			moveCount++;
			thread.nodes++;
			if (!isPV || moveCount > 1){
				score = -search(newDepth, ply+1, -alpha - 1, -alpha, ss+1, thread, limit);
			}
			if (isPV && (moveCount == 1 || score > alpha)){
				score = -search(newDepth, ply+1, -beta, -alpha, ss+1, thread, limit);
			}
			thread.board.unmakeMove(move);
			if (score > bestScore){
				bestScore = score;
				if (score > alpha){
					bestMove = move;
					ttFlag = TTFlag::EXACT;
					alpha = score;
					if (isPV){
						ss->pv.update(move, (ss+1)->pv);
					}
				}
			}
			if (score >= beta){
				ttFlag = TTFlag::BETA_CUT;
				break;
			}
			
		}
		if (!moveCount)
			return inCheck ? -MATE + ply : 0;

		*ttEntry = TTEntry(thread.board.hash(), ttFlag == TTFlag::FAIL_LOW ? ttEntry->move : bestMove, bestScore, ttFlag, depth);
		return bestScore;

	}

	int iterativeDeepening(Board &board, ThreadInfo &threadInfo, Limit limit, Searcher *searcher){
		//limit.start();
		threadInfo.abort.store(false);
		threadInfo.board = board;
		// TODO set nodes and stuff too
		bool isMain = threadInfo.type == ThreadType::MAIN;

		auto stack = std::make_unique<std::array<Stack, MAX_PLY+2>>();
		Stack *ss = reinterpret_cast<Stack*>(stack->data()+1);
		std::memset(stack.get(), 0, sizeof(Stack) * (MAX_PLY+2));

		PVList lastPV{};
		int score = -INFINITE;

		for (int depth=1;depth<=limit.depth;depth++){
			score = search(depth, 0, -INFINITE, INFINITE, ss, threadInfo, limit);
			auto aborted = [&]() {
				if (isMain)
					return limit.outOfTime() || threadInfo.abort.load(std::memory_order_relaxed);
				else
					return threadInfo.abort.load(std::memory_order_relaxed);
			};
			if (depth != 1 && aborted())
				break;
			if (!isMain)
				continue;
			lastPV = ss->pv;

			// Reporting
			uint64_t nodecnt = (*searcher).nodeCount();
			threadInfo.board.makeMove(lastPV.moves[0]);
			std::cout << "info depth " << depth << " score cp " << Evaluate(threadInfo.board, ~threadInfo.board.sideToMove()) << " nodes " << nodecnt << " nps " << nodecnt / (limit.timer.elapsed()+1) * 1000 << " pv ";
			threadInfo.board.unmakeMove(lastPV.moves[0]);
			for (int i=0;i<lastPV.length;i++)
				std::cout << lastPV.moves[i] << " ";
			std::cout << std::endl;
		}
		if (isMain){
			std::cout << "bestmove " << uci::moveToUci(lastPV.moves[0]) << std::endl;
		}
		threadInfo.abort.store(true, std::memory_order_relaxed);
		return score;
	}
}

void Searcher::start(Board &board, Search::Limit limit){
	mainInfo.nodes = 0;
	mainThread = std::thread(Search::iterativeDeepening, std::ref(board), std::ref(mainInfo), limit, this);
	for (int i=0;i<workerInfo.size();i++){	
		workerInfo[i].nodes = 0;
		workers.emplace_back(Search::iterativeDeepening, std::ref(board), std::ref(workerInfo[i]), limit, nullptr);
	}
}

void Searcher::stop(){
	abort.store(true, std::memory_order_relaxed);
	if (mainThread.joinable())
		mainThread.join();

	if (workers.size() > 0)
		for (std::thread &t : workers){
			if (t.joinable())
				t.join();
		}
	workers.clear();
}

void Searcher::initialize(int threads){
	threads-=1;
	stop();
	workerInfo.clear();
	for (int i=0;i<threads;i++){
		workerInfo.emplace_back(ThreadType::SECONDARY, TT, abort);
	}
}