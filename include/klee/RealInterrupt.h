#ifndef REAL_INTERRUPT_H
#define REAL_INTERRUPT_H

// #include "klee/Internal/Module/KInstIterator.h"

#include "stdlib.h"

// #include "llvm/Function.h"
#include "llvm/IR/Function.h"

using namespace llvm;

namespace klee{
  class KInstIterator;
}

namespace Inception{

  class RealInterrupt{
  public:

    RealInterrupt();

    ~RealInterrupt();

    static bool is_up(void);

    static Function* next_int_function(void);

  };
}

#endif
