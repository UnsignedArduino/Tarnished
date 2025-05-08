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
void MakeMove(Board &board, Accumulator &acc, Move &move){
	PieceType to = board.at<PieceType>(move.to());
	PieceType from = board.at<PieceType>(move.from());
	Color stm = board.sideToMove();
	board.makeMove(move);
	if (move.typeOf() == Move::CASTLING || move.typeOf() == Move::ENPASSANT || move.typeOf() == Move::PROMOTION){
		// For now just recalculate on special moves like these
		acc.refresh(board);
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

	if (move.typeOf() == Move::CASTLING || move.typeOf() == Move::ENPASSANT || move.typeOf() == Move::PROMOTION){
		// For now just recalculate on special moves like these
		acc.refresh(board);
	}
	else if (to != PieceType::NONE){
		acc.uncapture(stm, move.from(), from, move.to(), to, move.to(), from);
	}
	else {
		acc.quiet(stm, move.from(), from, move.to(), from);
	}
}