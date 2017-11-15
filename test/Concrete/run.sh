#!/bin/bash

KLEE="../../Debug+Asserts/bin/klee"
LLI="../../../tools/llvm/build_debug/Debug+Asserts/bin/lli"

find . -maxdepth 1 -name "*.ll" -exec ./ConcreteTest.py {} --klee $KLEE --lli $LLI \;
