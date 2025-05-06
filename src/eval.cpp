#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "eval.h"

Score EvaluatePiece(Board &board, PieceType pieceType, Bitboard area, Color color, int &phase){
	Score score = 0;
	Square sq(0);
	Bitboard bb = board.pieces(pieceType, color);

	while (bb){
		sq = Square(bb.pop());
		// PSQT and Material
		score += PSQT[(int)pieceType][color == Color::WHITE ? (sq^7).flip().index() : (sq^7).index()];
		score += pieceTypeValues[(int)pieceType];

		// Mobility (only for knights through queens)
		// Elo difference: 134.8 +/- 30.9
		if (pieceType != PieceType::PAWN && pieceType != PieceType::KING){
			Bitboard movement;
			// Xray included
			if (pieceType == PieceType::BISHOP)
				movement = attacks::bishop(sq, board.occ() ^ board.pieces(PieceType::QUEEN, color));
			else if (pieceType == PieceType::ROOK)
				movement = attacks::rook(sq, board.occ() ^ ( board.pieces(PieceType::ROOK, color) | board.pieces(PieceType::QUEEN, color) ));
			else if (pieceType == PieceType::QUEEN)
				movement = attacks::queen(sq, board.occ());
			else
				movement = attacks::knight(sq);

			int mob = (movement & area).count();

			// Also consider king saftey a little
			// Elo difference: 48.8 +/- 17.9
			int kingThreats = (movement & attacks::king(board.kingSq(~color))).count();

			score += mobilityBonus[(int)pieceType-1][mob];
			score += kingAttacks[(int)pieceType-1] * kingThreats;
		}


		// Phase Update
		phase += PhaseValues[(int)pieceType];

	}
	return score;
}

Score KingDanger(Board &board, Color color){
	Score score = 0;
	Bitboard safeline = Bitboard(color == Color::WHITE ? Rank::RANK_1 : Rank::RANK_8);
	int count = (~safeline & attacks::queen(board.kingSq(color), board.us(color) | board.pieces(PieceType::PAWN)  )  ).count();
	score += kingLineDanger[count];
	return score;
}

int Evaluate(Board &board, Color color){
	int phase = 0;
	Score eval = 0;

	// Material and Mobility
	Bitboard whitePawnAttacks = attacks::pawnLeftAttacks<Color::WHITE>(board.pieces(PieceType::PAWN, Color::WHITE)) | attacks::pawnRightAttacks<Color::WHITE>(board.pieces(PieceType::PAWN, Color::WHITE));
	Bitboard blackPawnAttacks = attacks::pawnLeftAttacks<Color::BLACK>(board.pieces(PieceType::PAWN, Color::BLACK)) | attacks::pawnRightAttacks<Color::BLACK>(board.pieces(PieceType::PAWN, Color::BLACK));
	// Mobility Area is everywhere that enemy pawns do not attack and is not occupied by the king ring
	Bitboard mobilityArea[2] = {
		~(blackPawnAttacks | attacks::king(board.kingSq(Color::WHITE))),
		~(whitePawnAttacks | attacks::king(board.kingSq(Color::BLACK)))
	};
	eval += EvaluatePiece(board, PieceType::PAWN, mobilityArea[0], Color::WHITE, phase)   - EvaluatePiece(board, PieceType::PAWN, mobilityArea[1], Color::BLACK, phase);
	eval += EvaluatePiece(board, PieceType::KNIGHT, mobilityArea[0], Color::WHITE, phase) - EvaluatePiece(board, PieceType::KNIGHT, mobilityArea[1], Color::BLACK, phase);
	eval += EvaluatePiece(board, PieceType::BISHOP, mobilityArea[0], Color::WHITE, phase) - EvaluatePiece(board, PieceType::BISHOP, mobilityArea[1], Color::BLACK, phase);
	eval += EvaluatePiece(board, PieceType::ROOK, mobilityArea[0], Color::WHITE, phase)   - EvaluatePiece(board, PieceType::ROOK, mobilityArea[1], Color::BLACK, phase);
	eval += EvaluatePiece(board, PieceType::QUEEN, mobilityArea[0], Color::WHITE, phase)  - EvaluatePiece(board, PieceType::QUEEN, mobilityArea[1], Color::BLACK, phase);
	eval += EvaluatePiece(board, PieceType::KING, mobilityArea[0], Color::WHITE, phase)   - EvaluatePiece(board, PieceType::KING, mobilityArea[1], Color::BLACK, phase);

	// King Line Danger
	eval += KingDanger(board, Color::WHITE) - KingDanger(board, Color::BLACK);

	// Bishop pair Bonus
	eval += board.pieces(PieceType::BISHOP, Color::WHITE).count() >= 2 ? bishopPair : 0;
	eval -= board.pieces(PieceType::BISHOP, Color::BLACK).count() >= 2 ? bishopPair : 0;

	Score phaseValue = (phase * 256 + 12) / 24;
	int score = (MgScore(eval) * phaseValue + (EgScore(eval) * (256 - phaseValue))) / 256;
	return (color == Color::WHITE ? score : -score) + 18; // Tempo Bonus
}