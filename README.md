# Tarnished

UCI Chess Engine written in C++ - the name is a reference to a certain video game protagonist.

## Usage

1. Clone the repository.
2. `mkdir build && cd build`
3. `cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release`
4. `cmake --build .`
5. Binary is at `build/tarnished.exe`

The [`Makefile`](Makefile) can also be used to build the engine. Just run `make` in the root directory. It is also used 
by OpenBench.

## Features

- Move Generation
    - Internally uses [chess-library](https://disservin.github.io/chess-library/)
- Evaluation
    - Tapered Evaluation
    - Piece Square Tables
    - Mobility Bonus
    - King Line Danger (inspired by Weiss)
- Search
    - Principle Variation Search
    - Quiescence Search
    - Iterative Deepening
    - Shared Transposition Table
    - Move Ordering
        - MVV-LVA
        - TT Move
        - Killer Move Heuristic 
        - Butterfly History Heuristic
    - Selectivity
        - Reverse Futility Pruning
        - Null Move Pruning
        - Late Move Reductions
        - Late Move Pruning
        - Terminal Conditions (Mate, Stalemate, 3fold...)
 - Misc
     - Lazy SMP (not tested thoroughly)

## Non-standard UCI Commands

- `print`
    - Prints out the board, side to move, castling rights, Zobrist Hash, etc
- `eval`
    - Prints the current position's static evaluation for the side to move
- `bench`
    - Runs an OpenBench style benchmark on 50 positions. Alternatively run `./tarnished bench`

## Credits

- Stockfish Discord Server
- [Weiss](https://github.com/TerjeKir/Weiss)
- [Stash](https://github.com/mhouppin/stash-bot)
- [Sirius](https://github.com/mcthouacbb/Sirius)
- [Prelude](https://git.nocturn9x.space/Quinniboi10/Prelude)
