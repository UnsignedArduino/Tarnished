#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "eval.h"

Score EvaluateMaterial(Board &board, PieceType pieceType, Color color, int &phase){
	Score score = 0;
	Square sq(0);
	Bitboard bb = board.pieces(pieceType, color);

	while (bb){
		sq = Square(bb.pop());
		//std::cout << "Piece type " << (int)pieceType << " Square " << (sq^7) << std::endl;
		score += PSQT[(int)pieceType][color == Color::WHITE ? (sq^7).flip().index() : (sq^7).index()];
		score += PieceTypeValues[(int)pieceType];
		phase += PhaseValues[(int)pieceType];
		//std::cout << bb << "\n";

	}
	return score;
}

int Evaluate(Board &board, Color color){
	int phase = 0;
	Score eval = 0;

	// Material
	eval += EvaluateMaterial(board, PieceType::PAWN, Color::WHITE, phase)   - EvaluateMaterial(board, PieceType::PAWN, Color::BLACK, phase);
	eval += EvaluateMaterial(board, PieceType::KNIGHT, Color::WHITE, phase) - EvaluateMaterial(board, PieceType::KNIGHT, Color::BLACK, phase);
	eval += EvaluateMaterial(board, PieceType::BISHOP, Color::WHITE, phase) - EvaluateMaterial(board, PieceType::BISHOP, Color::BLACK, phase);
	eval += EvaluateMaterial(board, PieceType::ROOK, Color::WHITE, phase)   - EvaluateMaterial(board, PieceType::ROOK, Color::BLACK, phase);
	eval += EvaluateMaterial(board, PieceType::QUEEN, Color::WHITE, phase)  - EvaluateMaterial(board, PieceType::QUEEN, Color::BLACK, phase);
	eval += EvaluateMaterial(board, PieceType::KING, Color::WHITE, phase)   - EvaluateMaterial(board, PieceType::KING, Color::BLACK, phase);

	Score phaseValue = (phase * 256 + 12) / 24;
	int score = (MgScore(eval) * phaseValue + (EgScore(eval) * (256 - phaseValue))) / 256;
	return (color == Color::WHITE ? score : -score);
}