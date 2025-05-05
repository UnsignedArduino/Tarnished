#include "search.h"
#include "searcher.h"
#include "eval.h"
#include "tt.h"

#include <algorithm>
#include <random>

using namespace chess;

// Searcher stuff



namespace Search {
	int16_t scoreMove(Move &move, Move &ttMove, ThreadInfo &thread){
		// MVV-LVA
		// Elo difference: 369.8 +/- 71.9 (#2)

		// TT Move
		// Elo difference: 154.8 +/- 33.8 (#4)
		return MVVLVA[thread.board.at<PieceType>(move.to())][thread.board.at<PieceType>(move.from())] + 60 * (move == ttMove);
	}
	bool scoreComparator(Move &a, Move &b){
		return a.score() > b.score();
	}
	void pickMove(Movelist &mvlst, int start){
		for (int i=start+1;i<mvlst.size();i++){
			if (mvlst[i].score() > mvlst[start].score()){
				std::iter_swap(mvlst.begin() + start, mvlst.begin() + i);
			}
		}
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

		// Move Scoring
		for (auto &move : moves){
			move.setScore(scoreMove(move, ttEntry->move, thread));
		}
		for (int m_ = 0;m_<moves.size();m_++){
			if (thread.abort.load(std::memory_order_relaxed))
				return bestScore;
			if (limit.outOfTime()){
				thread.abort.store(true, std::memory_order_relaxed);
				return bestScore;
			}

			pickMove(moves, m_);
			Move move = moves[m_];

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
		// Elo difference: 90.8 +/- 24.8 (#1)
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
		// Elo difference: 49.1 +/- 19.1 (#3)
		if (depth <= 6 && !root && eval - 80 * depth >= beta && std::abs(beta) < MATE)
			return eval;

		// Null Move Pruning
		// Failed
		// if (depth >= 3 && eval >= beta){
		// 	thread.board.makeNullMove();
		// 	int nmpScore = -search(depth-3, ply+1, -beta, -alpha, ss+1, thread, limit);
		// 	thread.board.unmakeNullMove();
		// 	if (nmpScore >= beta)
		// 		return nmpScore;
		// }


	move_loop:
		Move bestMove = Move();
		Movelist moves;

		movegen::legalmoves(moves, thread.board);

		// Move Scoring
		for (auto &move : moves){
			move.setScore(scoreMove(move, ttEntry->move, thread));
		}

		for (int m_ = 0;m_<moves.size();m_++){
			if (thread.abort.load(std::memory_order_relaxed))
				return bestScore;
			if (limit.outOfTime()){
				thread.abort.store(true, std::memory_order_relaxed);
				return bestScore;
			}
			pickMove(moves, m_);
			Move move = moves[m_];

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

	// int aspirationWindows(){
	// 	int delta = 8;
	// 	int alpha = -INFINITE;
	// 	int beta = INFINITE;
	// 	int aspDepth = depth;
	// 	if (depth >= 6){
	// 		int alpha = std::max(lastScore - delta, -INFINITE);
	// 		int beta = std::min(lastScore + delta, INFINITE);
	// 	}
		
	// 	while (true){
	// 		score = search(std::max(aspDepth, 1), 0, alpha, beta, ss, threadInfo, limit);
	// 		if (aborted())
	// 			break;
	// 		if (score <= alpha){
	// 			beta = (alpha + beta) / 2;
	// 			alpha = std::max(alpha - delta, -INFINITE);
	// 			aspDepth = depth; // Reset Depth
	// 		}
	// 		else {
	// 			if (score >= beta){
	// 				beta = std::min(beta + delta, INFINITE);
	// 				aspDepth = std::max(aspDepth - 1, depth - 5);
	// 			}
	// 			else
	// 				break;
	// 		}
	// 		delta += delta * 3 / 16;
	// 	}
	// }
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
		int lastScore = -INFINITE;

		for (int depth=1;depth<=limit.depth;depth++){
			
			auto aborted = [&]() {
				if (isMain)
					return limit.outOfTime() || threadInfo.abort.load(std::memory_order_relaxed);
				else
					return threadInfo.abort.load(std::memory_order_relaxed);
			};
			// Aspiration Windows 
			score = search(depth, 0, -INFINITE, INFINITE, ss, threadInfo, limit);
			// ---------------------
			
			if (depth != 1 && aborted())
				break;

			lastScore = score;

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