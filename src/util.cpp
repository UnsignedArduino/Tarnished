#include "external/chess.hpp"
#include "nnue.h"
#include "util.h"
#include <bit>
#include <vector>
#include <sstream>
#include <cassert>
#include <cstring>

// Accumulator wrapper
void MakeMove(Board &board, Accumulator &acc, Move &move){
	PieceType to = board.at<PieceType>(move.to());
	PieceType from = board.at<PieceType>(move.from());
	Color stm = board.sideToMove();
	board.makeMove(move);
	if (move.typeOf() == Move::ENPASSANT || move.typeOf() == Move::PROMOTION){
		// For now just recalculate on special moves like these
		acc.refresh(board);
	}
	else if (move.typeOf() == Move::CASTLING){
		Square king = move.from();
		Square kingTo = (king > move.to()) ? king - 2 : king + 2;
		Square rookTo = (king > move.to()) ? kingTo + 1 : kingTo - 1;
		// There are basically just 2 quiet moves now
		// Move king and move rook
		// Since moves are encoded as king takes rook, its very easy
		acc.quiet(stm, kingTo, PieceType::KING, move.from(), PieceType::KING);
		acc.quiet(stm, rookTo, PieceType::ROOK, move.to(), PieceType::ROOK);
	}
	else if (to != PieceType::NONE){
		acc.capture(stm, move.to(), from, move.from(), from, move.to(), to);
	}
	else
		acc.quiet(stm, move.to(), from, move.from(), from);

}

void UnmakeMove(Board &board, Accumulator &acc, Move &move){
	board.unmakeMove(move);

	PieceType to = board.at<PieceType>(move.to());
	PieceType from = board.at<PieceType>(move.from());
	Color stm = board.sideToMove();

	if (move.typeOf() == Move::ENPASSANT || move.typeOf() == Move::PROMOTION){
		// For now just recalculate on special moves like these
		acc.refresh(board);
	}
	else if (move.typeOf() == Move::CASTLING){
		Square king = move.from();
		Square kingTo = (king > move.to()) ? king - 2 : king + 2;
		Square rookTo = (king > move.to()) ? kingTo + 1 : kingTo - 1;
		// There are basically just 2 quiet moves now
		acc.quiet(stm, move.from(), PieceType::KING, kingTo, PieceType::KING);
		acc.quiet(stm, move.to(), PieceType::ROOK, rookTo, PieceType::ROOK);
	}
	else if (to != PieceType::NONE){
		acc.uncapture(stm, move.from(), from, move.to(), to, move.to(), from);
	}
	else {
		acc.quiet(stm, move.from(), from, move.to(), from);
	}
}