# Tarnished
UCI Chess Engine written in C++
The name is a reference to a certain video game protagonist

## Usage
You may build by cloning the repo and running `make`

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
        - Terminal Conditions (Mate, Stalemate, 3fold...)
 - Misc
     - Lazy SMP (not tested thoroughly)

## Credits
- Stockfish Discord Server
- [Weiss](https://github.com/TerjeKir/Weiss)
- [Stash](https://github.com/mhouppin/stash-bot)
- [Sirius](https://github.com/mcthouacbb/Sirius)
- [Prelude](https://git.nocturn9x.space/Quinniboi10/Prelude)