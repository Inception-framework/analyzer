#ifndef REAL_CONTEXT_SAVER_H
#define REAL_CONTEXT_SAVER_H

#include "klee/Internal/Module/KInstIterator.h"

namespace klee{
  class KInstIterator;
}

namespace Inception{

  class RealContextSaver{
  public:

    RealContextSaver();

    ~RealContextSaver();

    static void save(klee::KInstIterator &next_pc, klee::KInstIterator &current_pc);
  };
}

#endif
