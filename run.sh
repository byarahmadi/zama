#!/bin/bash
./lp.bin < $1
opt -load=./pass/emitcode.so -emit-code-pass IR.bc -o IR.bc

