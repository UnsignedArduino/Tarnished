#include <chrono>
#include "external/chess.hpp"

using namespace chess;
using namespace std::chrono;


class Perft {
   public:
    uint64_t perft(int depth) {
        Movelist moves;
        movegen::legalmoves(moves, board_);

        if (depth == 1) {
            return moves.size();
        }

        uint64_t nodes = 0;

        for (const auto& move : moves) {
            const auto gives_check = board_.givesCheck(move) != CheckType::NO_CHECK;
            board_.makeMove<true>(move);

            if (gives_check != board_.inCheck()) {
                throw std::runtime_error("givesCheck() and inCheck() are inconsistent");
            }

            nodes += perft(depth - 1);
            board_.unmakeMove(move);
        }

        return nodes;
    }

    void benchPerft(Board& board, int depth) {
        board_ = board;

        const auto t1    = high_resolution_clock::now();
        const auto nodes = perft(depth);
        const auto t2    = high_resolution_clock::now();
        const auto ms    = duration_cast<milliseconds>(t2 - t1).count();

        std::stringstream ss;

        // clang-format off
        ss << "depth " << std::left << std::setw(2) << depth
           << " time " << std::setw(5) << ms
           << " nodes " << std::setw(12) << nodes
           << " nps " << std::setw(9) << (nodes * 1000) / (ms + 1)
           << " fen " << std::setw(87) << board_.getFen();
        // clang-format on
        std::cout << ss.str() << std::endl;

    }

   private:
    Board board_;
};

int main(){
	Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
	Perft perft;
	perft.benchPerft(board, 5);
	return 0;
}