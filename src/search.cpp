#include "search.h"
#include "searcher.h"
#include "eval.h"
#include "tt.h"
#include "util.h"

#include <algorithm>
#include <random>

using namespace chess;

// Searcher stuff



namespace Search {
	int16_t scoreMove(Move &move, Move &ttMove, Move &killer, ThreadInfo &thread){
		// MVV-LVA
		// TT Move
		// Killer Move Heuristic
		// Butterfly History Heuristic

		PieceType to = thread.board.at<PieceType>(move.to());
		if (move == ttMove){
			return 32511 + 60;
		}
		else if (to != PieceType::NONE) {
			// Max 16 bit int - 256 + MVVLVA
			// https://rustic-chess.org/search/ordering/killers.html
			return 32511 + MVVLVA[to][thread.board.at<PieceType>(move.from())];
		}
		else if (move == killer){
			return 32511 - 10;
		}
		else {
			// Quiet non killer
			// Butterfly
			return 32511 - 16384 + thread.getHistory(thread.board.sideToMove(), move);
		}
		return 0;
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
	template<bool isPV>
	int qsearch(int ply, int alpha, const int beta, Stack *ss, ThreadInfo &thread, Limit &limit){
		//bool isPV = alpha != beta - 1;
		TTEntry *ttEntry = thread.TT.getEntry(thread.board.hash());
		bool ttHit = ttEntry->zobrist == thread.board.hash();
		if (!isPV && ttHit
			&& (ttEntry->flag == TTFlag::EXACT 
				|| (ttEntry->flag == TTFlag::BETA_CUT && ttEntry->score >= beta)
				|| (ttEntry->flag == TTFlag::FAIL_LOW && ttEntry->score <= alpha))){
			//thread.ttHits ++;
			return ttEntry->score;
		}

		//int score = Eval::Evaluate(thread.board, thread.board.sideToMove());
		int score = network.inference(&thread.board, &thread.accumulator);
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
			// Qsearch doesnt have killers
			// Still pass to make compiler happy
			move.setScore(scoreMove(move, ttEntry->move, ss->killer, thread));
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

			//thread.board.makeMove(move);
			MakeMove(thread.board, thread.accumulator, move);
			thread.nodes++;
			score = -qsearch<isPV>(ply+1, -beta, -alpha, ss+1, thread, limit);
			//thread.board.unmakeMove(move);
			UnmakeMove(thread.board, thread.accumulator, move);

			if (score > bestScore){
				bestScore = score;
				if (score > alpha){
					alpha = score;
					// if (isPV)
					// 	ss->pv.update(move, (ss+1)->pv);
				}
			}
			if (score >= beta){
				break;
			}
		}
		return bestScore;

	}
	template<bool isPV>
	int search(int depth, int ply, int alpha, int beta, Stack *ss, ThreadInfo &thread, Limit &limit){
		//bool isPV = alpha != beta - 1;
		bool root = ply == 0;
		if (isPV) 
			ss->pv.length = 0;
		if (depth <= 0){
			return qsearch<isPV>(ply, alpha, beta, ss, thread, limit);
		}

		// Terminal Conditions (and checkmate)
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
		//eval = Eval::Evaluate(thread.board, thread.board.sideToMove());
		eval = network.inference(&thread.board, &thread.accumulator);
		// Reverse Futility Pruning
		if (depth <= 6 && !root && eval - 80 * depth >= beta && std::abs(beta) < MATE)
			return eval;

		// Null Move Pruning
		if (depth >= 3 && eval >= beta){
			thread.board.makeNullMove();
			int nmpScore = -search<false>(depth-3, ply+1, -beta, -alpha, ss+1, thread, limit);
			thread.board.unmakeNullMove();
			if (nmpScore >= beta)
				return nmpScore;
		}


	move_loop:
		Move bestMove = Move();
		Movelist moves;
		Movelist seenQuiets;

		movegen::legalmoves(moves, thread.board);

		// Move Scoring
		for (auto &move : moves){
			move.setScore(scoreMove(move, ttEntry->move, ss->killer, thread));
		}

		// Other vars
		bool skipQuiets = false;
		for (int m_ = 0;m_<moves.size();m_++){
			if (thread.abort.load(std::memory_order_relaxed))
				return bestScore;
			if (limit.outOfTime()){
				thread.abort.store(true, std::memory_order_relaxed);
				return bestScore;
			}
			pickMove(moves, m_);
			Move move = moves[m_];
			bool isQuiet = thread.board.at<PieceType>(move.to()) == PieceType::NONE;

			if (isQuiet && skipQuiets)
				continue;
			if (isQuiet)
				seenQuiets.add(move);

			
			if (bestScore > -INFINITE + MAX_PLY){
				// Late Move Pruning
				if (!skipQuiets && isQuiet && moveCount >= 6 * depth * depth){
					skipQuiets = true;
					continue;
				}
			}

			//thread.board.makeMove<true>(move);
			MakeMove(thread.board, thread.accumulator, move);
			int newDepth = depth-1;
			moveCount++;
			thread.nodes++;

			// Late Move Reduction
			if (depth > 2 && moveCount > 2 && isQuiet && !thread.board.inCheck()){
				score = -search<false>(newDepth-1, ply+1, -alpha - 1, -alpha, ss+1, thread, limit);
				// Re-search at normal depth
				if (score > alpha)
					score = -search<false>(newDepth, ply+1, -alpha - 1, -alpha, ss+1, thread, limit);
			}
			else if (!isPV || moveCount > 1){
				score = -search<false>(newDepth, ply+1, -alpha - 1, -alpha, ss+1, thread, limit);
			}
			if (isPV && (moveCount == 1 || score > alpha)){
				score = -search<isPV>(newDepth, ply+1, -beta, -alpha, ss+1, thread, limit);
			}
			//thread.board.unmakeMove(move);
			UnmakeMove(thread.board, thread.accumulator, move);
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
				ss->killer = isQuiet ? bestMove : Move::NO_MOVE;
				// Butterfly History
				if (isQuiet){
					thread.updateHistory(thread.board.sideToMove(), move, 20 * depth * depth);
				}
				for (const Move quietMove : seenQuiets){
					if (quietMove == move)
						continue;
					thread.updateHistory(thread.board.sideToMove(), quietMove, -20 * depth * depth);
				}
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
		int lastScore = -INFINITE;

		for (int depth=1;depth<=limit.depth;depth++){
			
			auto aborted = [&]() {
				if (isMain)
					return limit.outOfTime() || threadInfo.abort.load(std::memory_order_relaxed);
				else
					return threadInfo.abort.load(std::memory_order_relaxed);
			};
			// Aspiration Windows (WIP)
			if (false){
				// int delta = 12; // 12 probably gains
				// int alpha  = std::max(lastScore - delta, -INFINITE);
				// int beta = std::max(lastScore + delta, INFINITE);
				// while (!aborted()){
				// 	score = search<true>(depth, 0, alpha, beta, ss, threadInfo, limit);
				// 	if (score <= alpha || score >= beta) {
				// 		alpha = -INFINITE;
				// 		beta = INFINITE;
				// 	}
				// 	else
				// 		break;
				// }
			}
			else
				score = search<true>(depth, 0, -INFINITE, INFINITE, ss, threadInfo, limit);
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
			std::cout << "info depth " << depth << " score cp " << network.inference(&threadInfo.board, &threadInfo.accumulator) << " nodes " << nodecnt << " nps " << nodecnt / (limit.timer.elapsed()+1) * 1000 << " pv ";
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

	// Benchmark for OpenBench
	void bench(){
	    int64_t totalNodes = 0;
	    int64_t totalMS = 0;

	    std::cout << "Benchmark started at depth " << (int)BENCH_DEPTH << std::endl;
	    // Thanks Prelude
	    std::array<std::string, 50> fens = {"r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
	                              "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
	                              "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
	                              "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
	                              "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
	                              "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
	                              "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
	                              "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
	                              "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
	                              "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
	                              "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
	                              "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
	                              "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
	                              "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
	                              "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
	                              "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
	                              "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
	                              "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
	                              "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
	                              "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
	                              "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
	                              "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
	                              "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
	                              "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
	                              "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
	                              "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
	                              "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
	                              "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
	                              "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
	                              "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
	                              "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
	                              "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
	                              "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
	                              "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
	                              "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/3R4 w - - 6 54",
	                              "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
	                              "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
	                              "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
	                              "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
	                              "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
	                              "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
	                              "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
	                              "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
	                              "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
	                              "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
	                              "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
	                              "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
	                              "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
	                              "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
	                              "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"};

	    TimeLimit timer = TimeLimit();

	    for (auto fen : fens){
	        timer.start();
	        Board board(fen);
	        std::atomic<bool> benchAbort(false);
	        TTable TT;
	        Search::ThreadInfo thread(ThreadType::SECONDARY, TT, benchAbort);

	        Search::Limit limit = Search::Limit();
	        limit.depth = (int64_t)BENCH_DEPTH; limit.movetime = 0; limit.ctime = 0;
	        limit.start();

	        Search::iterativeDeepening(board, thread, limit, nullptr);

	        int ms = timer.elapsed();
	        totalMS += ms;
	        totalNodes += thread.nodes;
	        
	        std::cout << "-----------------------------------------------------------------------" << std::endl;
	        std::cout << "FEN: " << fen << std::endl;
	        std::cout << "Nodes: " << thread.nodes << std::endl;
	        std::cout << "Time: " << ms << "ms" << std::endl;
	        std::cout << "-----------------------------------------------------------------------\n" << std::endl;
	    }
	    std::cout << "Completed Benchmark" << std::endl;
	    std::cout << "Total Nodes: " << totalNodes << std::endl;
	    std::cout << "Elapsed Time: " << totalMS << "ms" << std::endl;
	    int nps = static_cast<int64_t>((totalNodes / totalMS) * 1000);
	    std::cout << "Average NPS: " << nps << std::endl;
	    std::cout << totalNodes << " nodes " << nps << " nps" << std::endl;
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