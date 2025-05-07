
# Tarnished
UCI Chess Engine written in C++

The name is a reference to a certain video game protagonist

As of right now, all tests have been conducted on my personal PC with the use of [cutechess-cli](https://github.com/cutechess/cutechess/tree/master) but will eventually migrate [OpenBench](https://github.com/AndyGrant/OpenBench)

HCE evaluation values were taken from my microcontroller chess engine [MemorixV2](https://github.com/Bobingstern/MemorixV2) which were computed via [Texel-Tuning](https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf)

## Strength
So far, Tarnished can defeat [Stash-19.0](https://gitlab.com/mhouppin/stash-bot) at `8+0.08` and `40+0.4` time controls which is estimated ~2471 CCRL Blitz. These are the results after ~900 games on an `Intel(R) Core(TM) i7-10700F`

`Engine | tarnished vs stash-19.0`

```
TC     | 8+0.08
Elo    | 35.0 +- 19.7
Games  | N: 995 W: 462 L: 363 D: 171 [0.550]
```
```
TC     | 40+0.4
Elo    | 17.1 +- 13.8
Games  | N: 1950 W: 830 L: 734 D: 386 [0.525]
``` 

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
- Evaluation
    - Tapered Evaluation
    - Piece Square Tables
    - Mobility Bonus
    - King Line Danger (inspired by Weiss)
    - King Threats
    - Bishop Pair
    - Tempo Bonus
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
- [Prelude](https://git.nocturn9x.space/Quinniboi10/Prelude)