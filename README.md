# Nand2Tetris II


Another attempt at [Nand2Tetris](https://github.com/aceiii/nand2tetris)

This time I'll be working through the book 'The Elements of Computer Systems: Building A Modern Computer from First Principles',
but instead of just performing the normal problems and verifying it using the provided simulators I will build them myself.

The goal of this project will be to build a VM for the CPU, the set assembler, compiler, and a GUI to run everything.


## Build

```
git clone --recursive https://github.com/aceiii/nand2tetris-II.git
cd nand2tetris-II

cmake --preset Debug
cmake --build --preset Debug


# run application
./build/nand2tetris

```
