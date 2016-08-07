# Persper Transactional Memory Compiler (PTMC)

[![Build Status](https://travis-ci.org/Persper/PTMC.svg?branch=master)](https://travis-ci.org/Persper/PTMC)

We revive DTMC (http://www.velox-project.eu/software/dtmc) to support
programming for transactional persistent memory.

DTMC was built on LLVM 2.8, which unfortunately does not support latest Intel
processors (since Sandy Bridge). For now, our workaround is using DTMC to
compile a program to IR, and using LLVM 2.9 to convert IR to native code.

Steps for building the infrastructure can be found in our build script
[.travis.yml](https://github.com/Persper/PTMC/blob/master/.travis.yml). Due to
the above workaround, we have to make Tanger in two rounds.

To build a program, please follow the two steps below. Suppose $TANGER
refers to the Release directory in tanger\_R3.

1. `llvm-gcc -I $TANGER/../include -emit-llvm -fgnu-tm -c -o program.bc program.c`

2. `$TANGER/bin/tmlink -load=$TANGER/lib/libtanger.so -stmsupport=$TANGER/lib/ -stmlib=$TANGER/lib/ -o program program.bc`
