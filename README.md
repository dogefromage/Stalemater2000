# Stalemater2000 - a C++ chess engine from scratch with NNUE

Stalemater2000 searches for the best chess move in a position using iterative deepening framework like most popular chess engines. The engine uses a simple two layer [NNUE (efficiently updatable neural network)](https://www.chessprogramming.org/NNUE) which predict the evaluation of the leaf nodes in the search space given the board squares as input features. The engine is written in C++ with no dependencies and runs in the command line featuring the [UCI protocol for inputs and communication with other software](https://www.shredderchess.com/chess-features/uci-universal-chess-interface.html). The NNUE model is trained using torch and uses the [lichess.org evaluation dataset](https://database.lichess.org/#evals).

## Skill level
It beats stockfish lvl 4 on lichess but I don't know if that's something to be proud of :/

## Usage

### Input position and display board using [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation):

```
position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1
d
```
```
  -------------------
8 | r n b q k b n r |
7 | p p p p p p p p |
6 | . . . . . . . . |
5 | . . . . . . . . |
4 | . . . . P . . . |
3 | . . . . . . . . |
2 | P P P P . P P P |
1 | R N B Q K B N R |
  -------------------
    a b c d e f g h  

Halfmove clock: 0
Fullmove number: 1

FEN: rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq-
Hash: 2824af1b2a2d7300

Is legal: 1
Checks:
```

### Do a [perft analysis](https://www.chessprogramming.org/Perft_Results) to validate the engine by counting legal subpositions: 
```
go perft 5
```
```
b8a6 -> 398321
b8c6 -> 475842
g8f6 -> 474457
g8h6 -> 400579
a7a6 -> 370351
b7b6 -> 430756
c7c6 -> 456786
d7d6 -> 639932
e7e6 -> 822484
f7f6 -> 342008
g7g6 -> 442824
h7h6 -> 363576
a7a5 -> 442101
b7b5 -> 411896
c7c5 -> 482325
d7d5 -> 788468
e7e5 -> 728887
f7f5 -> 427835
g7g5 -> 426723
h7h5 -> 445481
Total: 9771632
```

### Find the best move in the current position up to a depth of 7 halfmoves:
```bash
go depth 7
```
```bash
info depth 1 score cp 0 nodes 21 nps 512195 pv b1a3
info depth 2 score cp 0 nodes 81 nps 1538461 pv b1a3 b8a6
info depth 3 score cp 0 nodes 667 nps 1693641 pv b1a3 b8a6 g1f3
info depth 4 score cp 0 nodes 2162 nps 1752637 pv b1a3 b8a6 g1f3 a6b4
info depth 5 score cp 100 nodes 26955 nps 1876835 pv b2b3 b8a6 c1b2 a6b4 b2g7
info depth 6 score cp -100 nodes 201096 nps 1342872 pv b1a3 b7b6 g1f3 c8a6 a3b1 a6e2
info depth 7 score cp 100 nodes 998975 nps 1180470 pv b1c3 b8a6 g1f3 a6b4 c3b5 b4d5 b5a7
bestmove b1c3
```

## Useful links
Everything you'd ever would want to know about chess programming can be found on the [chess programming wiki](https://www.chessprogramming.org). It has lots of pseudocode and details 
about both historic and leading-edge approaches.

The bitboard logic I used was really well explained in [this Youtube series by Logic Crazy Chess](https://www.youtube.com/watch?v=V_2-LOvr5E8&list=PLQV5mozTHmacMeRzJCW_8K3qw2miYqd0c).

## TODO: Unfinished work
* Quantize NNUE model such that it uses integer arithmetic and SIMD instead of lousy float32
* Do profiling to get performance up (I'm sure there are a lot of redundant copies in the search algorithm)
* Find better dataset with less chaotic positions for better model (loss on current model is really high, but performs alright)

## Future ideas
* Support for all UCI search parameters
* [Aspiration windows](https://www.chessprogramming.org/Aspiration_Windows)
* [Parallel Search](https://www.chessprogramming.org/Lazy_SMP)
* [Magic Boards](https://www.chessprogramming.org/Magic_Bitboards)