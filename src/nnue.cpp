#include "nnue.h"
#include "search.h"

#include <fstream>
#include <format>
#include <algorithm>
#include <immintrin.h>
#include <random>


int16_t NNUE::ReLU_(int16_t x){
	return x < 0 ? 0 : x;
}

int16_t NNUE::CReLU_(int16_t x){
	if (x < 0)
		return 0;
	return x > QA ? QA : x;
}

int32_t NNUE::SCReLU_(int16_t x){
	const int32_t c = CReLU_(x);
	return c * c;
}

int NNUE::feature(Color persp, Color color, PieceType p, Square sq){
	int ci = persp == color ? 0 : 1;
	int sqi = persp == Color::BLACK ? (sq).flip().index() : (sq).index();
	return ci * 64 * 6 + (int)p * 64 + sqi; // Index of the feature
}

void NNUE::load(const std::string &file){
	std::ifstream stream(file, std::ios::binary);

	for (int i=0;i<H1.size();++i){
		H1[i] = readLittleEndian<int16_t>(stream);
	}
	for (int i=0;i<H1Bias.size();++i){
		H1Bias[i] = readLittleEndian<int16_t>(stream);
		//std::cout << "Bias " << H1Bias[i] << std::endl;
	}
	for (int i=0;i<OW.size(); ++i) {
		//for (int j=0;j<OW[i].size();j++)
		OW[i] = readLittleEndian<int16_t>(stream);
	}
	outputBias = readLittleEndian<int16_t>(stream);
}

void NNUE::randomize(){
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(-500, 500);
	for (int i=0;i<H1.size();++i){
		H1[i] = distrib(gen);
	}
	for (int i=0;i<H1Bias.size();++i){
		H1Bias[i] = distrib(gen);
	}
	for (int i=0;i<OW.size(); ++i) {
		OW[i] = distrib(gen);
	}
	outputBias = distrib(gen);
}

int NNUE::inference(Board *board, Accumulator *accumulator){

	Color stm = board->sideToMove();

	const std::array<int16_t, HL_N> &accumulatorSTM = stm == Color::WHITE ? accumulator->white : accumulator->black;
	const std::array<int16_t, HL_N> &accumulatorOPP = stm == Color::BLACK ? accumulator->white : accumulator->black;

	// Debug for now
	// for (int i=0;i<HL_N;i++){
	// 	std::cout << "STM " << accumulatorSTM[i] << " OPP " << accumulatorOPP[i] << std::endl;
	// }

	int64_t eval = 0;
	// https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md
	for (int i=0;i<HL_N;i++){
		if (ACTIVATION == SCReLU){
			eval += SCReLU_(accumulatorSTM[i]) * OW[i];
			eval += SCReLU_(accumulatorOPP[i]) * OW[HL_N+i];
			//printf("OutputW1 %hd OutputW2 %hd Eval %lld\n", OW[i], OW[HL_N+i], eval);
		}
	}
	if (ACTIVATION == SCReLU)
		eval /= QA;

	eval += outputBias;

	//printf("EVAL %lld\n", (eval * NNUE_SCALE) / (QA * QB));
	return (eval * NNUE_SCALE) / (QA * QB);


}

// ------ Accumulator -------

void Accumulator::refresh(Board &board){
	Bitboard whiteBB = board.us(Color::WHITE);
	Bitboard blackBB = board.us(Color::BLACK);

	// Obviously the bias is commutative so just add it first
	white = network.H1Bias; 
	black = network.H1Bias;

	while (whiteBB){
		Square sq = whiteBB.pop();

		// White features for both perspectives
		int wf = NNUE::feature(Color::WHITE, Color::WHITE, board.at<PieceType>(sq), sq); 
		int bf = NNUE::feature(Color::BLACK, Color::WHITE, board.at<PieceType>(sq), sq); 

		for (int i=0;i<HL_N;i++){
			// Do the matrix mutliply for the next layer
			white[i] += network.H1[wf * HL_N + i];
			black[i] += network.H1[bf * HL_N + i];
		}
	}

	while (blackBB){
		Square sq = blackBB.pop();

		// Black features for both perspectives
		int wf = NNUE::feature(Color::WHITE, Color::BLACK, board.at<PieceType>(sq), sq); 
		int bf = NNUE::feature(Color::BLACK, Color::BLACK, board.at<PieceType>(sq), sq); 

		for (int i=0;i<HL_N;i++){
			// Do the matrix mutliply for the next layer
			white[i] += network.H1[wf * HL_N + i];
			black[i] += network.H1[bf * HL_N + i];
		}
	}

	// for (int i=0;i<HL_N;i++){
	// 	//std::cout << "STMw " << white[i] << " OPPb " << black[i] << std::endl;
	// }
}


// Quiet Accumulation
void Accumulator::quiet(Color stm, Square add, PieceType addPT, Square sub, PieceType subPT){
	int addW = NNUE::feature(Color::WHITE, stm, addPT, add);
	int addB = NNUE::feature(Color::BLACK, stm, addPT, add);

	int subW = NNUE::feature(Color::WHITE, stm, subPT, sub);
	int subB = NNUE::feature(Color::BLACK, stm, subPT, sub);

	for (int i=0;i<HL_N;i++){
		white[i] += network.H1[addW * HL_N + i];
		black[i] += network.H1[addB * HL_N + i];

		white[i] -= network.H1[subW * HL_N + i];
		black[i] -= network.H1[subB * HL_N + i];
	}
}
// Capture Accumulation
void Accumulator::capture(Color stm, Square add, PieceType addPT, Square sub1, PieceType subPT1, Square sub2, PieceType subPT2){
	int addW = NNUE::feature(Color::WHITE, stm, addPT, add);
	int addB = NNUE::feature(Color::BLACK, stm, addPT, add);

	int subW1 = NNUE::feature(Color::WHITE, stm, subPT1, sub1);
	int subB1 = NNUE::feature(Color::BLACK, stm, subPT1, sub1);

	int subW2 = NNUE::feature(Color::WHITE, ~stm, subPT2, sub2);
	int subB2 = NNUE::feature(Color::BLACK, ~stm, subPT2, sub2);

	for (int i=0;i<HL_N;i++){
		white[i] += network.H1[addW * HL_N + i];
		black[i] += network.H1[addB * HL_N + i];

		white[i] -= network.H1[subW1 * HL_N + i];
		black[i] -= network.H1[subB1 * HL_N + i];

		white[i] -= network.H1[subW2 * HL_N + i];
		black[i] -= network.H1[subB2 * HL_N + i];
	}
}

// Undo Capture
void Accumulator::uncapture(Color stm, Square add1, PieceType addPT1, Square add2, PieceType addPT2, Square sub, PieceType subPT){
	int addW1 = NNUE::feature(Color::WHITE, stm, addPT1, add1);
	int addB1 = NNUE::feature(Color::BLACK, stm, addPT1, add1);

	int addW2 = NNUE::feature(Color::WHITE, ~stm, addPT2, add2);
	int addB2 = NNUE::feature(Color::BLACK, ~stm, addPT2, add2);

	int subW = NNUE::feature(Color::WHITE, stm, subPT, sub);
	int subB = NNUE::feature(Color::BLACK, stm, subPT, sub);

	for (int i=0;i<HL_N;i++){
		white[i] += network.H1[addW1 * HL_N + i];
		black[i] += network.H1[addB1 * HL_N + i];

		white[i] += network.H1[addW2 * HL_N + i];
		black[i] += network.H1[addB2 * HL_N + i];

		white[i] -= network.H1[subW * HL_N + i];
		black[i] -= network.H1[subB * HL_N + i];
	}
}

// Castle Accumulation
void Accumulator::castle(Color stm, Square add1, PieceType addPT1, Square add2, PieceType addPT2, Square sub1, PieceType subPT1, Square sub2, PieceType subPT2){
	int addW1 = NNUE::feature(Color::WHITE, stm, addPT1, add1);
	int addB1 = NNUE::feature(Color::BLACK, stm, addPT1, add1);

	int addW2 = NNUE::feature(Color::WHITE, stm, addPT2, add2);
	int addB2 = NNUE::feature(Color::BLACK, stm, addPT2, add2);

	int subW1 = NNUE::feature(Color::WHITE, stm, subPT1, sub1);
	int subB1 = NNUE::feature(Color::BLACK, stm, subPT1, sub1);

	int subW2 = NNUE::feature(Color::WHITE, stm, subPT2, sub2);
	int subB2 = NNUE::feature(Color::BLACK, stm, subPT2, sub2);

	for (int i=0;i<HL_N;i++){
		white[i] += network.H1[addW1 * HL_N + i];
		black[i] += network.H1[addB1 * HL_N + i];

		white[i] += network.H1[addW2 * HL_N + i];
		black[i] += network.H1[addB2 * HL_N + i];

		white[i] -= network.H1[subW1 * HL_N + i];
		black[i] -= network.H1[subB1 * HL_N + i];

		white[i] -= network.H1[subW2 * HL_N + i];
		black[i] -= network.H1[subB2 * HL_N + i];
	}
}

