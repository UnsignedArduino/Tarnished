#include <chrono>
#include "external/chess.hpp"
#include "search.h"
#include "searcher.h"
#include "eval.h"
#include "uci.h"
#include "timeman.h"

using namespace chess;
using namespace std::chrono;


// Thanks Weiss
void ParseTimeControl(char *str, Color color, Search::Limit &limit) {

    // Read in relevant search constraints
    int64_t mtime = 0;
    int64_t ctime = 0;
    int64_t depth = 0;

    SetLimit(str, "movetime",  &mtime); 
    SetLimit(str, "depth",     &depth);

    if (mtime == 0 && depth == 0){
        SetLimit(str, color == Color::WHITE ? "wtime" : "btime", &ctime);    
    }
    
    limit.ctime = ctime;  
    limit.movetime = mtime;
    limit.depth = depth;

    limit.start();
}
  
void UCIPosition(Board &board, char *str) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    if (BeginsWith(str, "position fen")){
        std::string_view fen = str+13;
        board.setFen(fen);
    }
    else { 
        board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } 

    // Check if there are moves to be made from the initial position
    if ((str = strstr(str, "moves")) == NULL)
        return;

    // Loop over the moves and make them in succession
    char *move = strtok(str, " ");
    while ((move = strtok(NULL, " "))) {

        // Parse and make move
        std::string m = move;
        board.makeMove(uci::uciToMove(board, m));
    }
    // Reset the state
    //std::cout << "Repeat " << board.isRepetition(1) << std::endl;
    //board.setFen(board.getFen());

}

static int HashInput(char *str) {
    int hash = 0;
    int len = 1;
    while (*str && *str != ' ')
        hash ^= *(str++) ^ len++;
    return hash;
};

void UCISetOption(Searcher &searcher, char *str) {

    // Sets the size of the transposition table
    if (OptionName(str, "Hash")) {
        searcher.TT.resize((uint64_t)atoi(OptionValue(str)));
        std::cout << "Hash Table successfully resized" << std::endl;
    // Sets number of threads to use for searching
    } else if (OptionName(str, "Threads")) {
        searcher.initialize(atoi(OptionValue(str)));
    }
}
void UCIInfo(){
    std::cout << "id name Tarnished\n";
    std::cout << "id author Anik Patel\n";
    std::cout << "option name Hash type spin default 16 min 2 max 65536\n";
    std::cout << "option name Threads type spin default 1 min 1 max 256\n";
    std::cout << "uciok" << std::endl; 
}

void UCIGo(Searcher &searcher, Board &board, char *str){
    searcher.stop();

    Search::Limit limit = Search::Limit();
    ParseTimeControl(str, board.sideToMove(), limit);

    searcher.start(board, limit);
    //searcher.stop();
}   



int main(int agrc, char *argv[]){
    //r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
	Board board = Board();
    Searcher searcher = Searcher();
    searcher.initialize(1); // Default one thread

    if (agrc > 1){
        std::string arg = argv[1];
        if (arg == "bench")
            Search::bench();
    }
    char str[INPUT_SIZE];
    while (GetInput(str)) {
        switch (HashInput(str)) {
            case GO         : UCIGo(searcher, board, str);                break;
            case UCI        : UCIInfo();                                  break;
            case ISREADY    : std::cout << "readyok" << std::endl;        break;
            case POSITION   : UCIPosition(board, str);                    break;
            case SETOPTION  : UCISetOption(searcher, str);                break;
            case UCINEWGAME : searcher.reset();                           break;
            case STOP       : searcher.stop();                            break;
            case QUIT       : searcher.stop();                            return 0;

            // Non Standard
            case PRINT      : std::cout << board << std::endl;            break;
            case EVAL       : std::cout << Evaluate(board, board.sideToMove()) << std::endl; break;
            case BENCH      : Search::bench();                            break;

        }
    }

	return 0;
}