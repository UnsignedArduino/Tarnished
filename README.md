

<p align="center">
</p>
<h1 align="center">Tarnished</h1>

UCI Chess Engine written in C++ featuring NNUE evaluation trained from scratch. The name is a reference to a certain video game protagonist

As of right now, all tests and data generation have been conducted on my personal PCs with the use of [cutechess-cli](https://github.com/cutechess/cutechess/tree/master) but will eventually migrate to [OpenBench](https://github.com/AndyGrant/OpenBench)


## Building
It seems like the `Makefile` is slightly faster than using `CMake` but you may use whichever one you wish. Make sure to have an NNUE file under `network/latest.bin` if you plan on building the project. Tarnished makes use of [incbin](https://github.com/graphitemaster/incbin) to embed the file into the executable itself, removing the need to carry an external network along with it. To build with `make` you may 
1. Clone the repository.
2. `make`
3. Binary is at `tarnished.exe`

Alternatively, with `CMake`

1. Clone the repository.
2. `mkdir build && cd build`
3. `cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release`
4. `cmake --build .`
5. Binary is at `build/Tarnished.exe`

## Features

- Move Generation
    - Internally uses [chess-library](https://disservin.github.io/chess-library/)
- NNUE `(768->64)x2->1x8`
    - Trained with [bullet](https://github.com/jw1912/bullet)
    - Self generated training data
    - 5000 soft nodes
    - 8 random plies
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
 - `datagen name Threads value <threads>`
     - Begins data generation with the specified number of threads
     - Send me your data!

## Credits
- Stockfish Discord Server
- [Weiss](https://github.com/TerjeKir/Weiss)
- [Stash](https://github.com/mhouppin/stash-bot)
- [Sirius](https://github.com/mcthouacbb/Sirius) 
- [Stormphrax](https://github.com/Ciekce/Stormphrax)
- [Prelude](https://git.nocturn9x.space/Quinniboi10/Prelude) (Especially for base NNUE code)
- [Bullet](https://github.com/jw1912/bullet)