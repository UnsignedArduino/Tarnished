
<p align="center">
</p>
<h1 align="center">Tarnished</h1>

UCI Chess Engine written in C++ featuring NNUE evaluation. The name is a reference to a certain video game protagonist

As of right now, all tests have been conducted on my personal PC with the use of [cutechess-cli](https://github.com/cutechess/cutechess/tree/master) but will eventually migrate to [OpenBench](https://github.com/AndyGrant/OpenBench)


## Usage
It seems like the `Makefile` is slightly faster than using `CMake` but you may use whichever one you wish 
1. Clone the repository.
2. `make`
3. Binary is at `tarnished.exe`

Alternatively,

1. Clone the repository.
2. `mkdir build && cd build`
3. `cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release`
4. `cmake --build .`
5. Binary is at `build/Tarnished.exe`

## Features

- Move Generation
    - Internally uses [chess-library](https://disservin.github.io/chess-library/)
- NNUE `(768->64->1)`
    - Pre-trained network
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
     - Lazy SMP (functional but not tested thoroughly)

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
- [Prelude](https://git.nocturn9x.space/Quinniboi10/Prelude) (Especially for base NNUE code)