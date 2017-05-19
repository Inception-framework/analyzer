#ifndef REAL_INTERRUPT_H
#define REAL_INTERRUPT_H

#ifndef WATCHER
#define WATCHER
typedef void (*Watcher)(int);
#endif

#if defined(__cplusplus)
extern "C" {
#endif

void trace_init(void *opaque, Watcher watcher);

#if defined(__cplusplus)
}
#endif

// #include "klee/Internal/Module/KInstIterator.h"

#include "stdlib.h"

// #include "llvm/Function.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/StringRef.h"

#include <iostream>
#include "map"
#include "list"
#include <queue>

using namespace llvm;

namespace klee{
  class KInstIterator;
}

namespace Inception{

  class Interrupt{

    public :
      Interrupt(llvm::StringRef _handlerName, uint32_t _id, uint32_t _group_priority, uint32_t _internal_priority) :
        handlerName(_handlerName), id(_id), group_priority(_group_priority), internal_priority(_internal_priority) {}

      Interrupt();

      ~Interrupt();

      llvm::StringRef handlerName;

      uint32_t id;

      uint32_t group_priority;

      uint32_t internal_priority;
  };

  class InterruptComparator{
  public:
    bool operator() (Interrupt* a, Interrupt* b) {
      if(a->group_priority > b->group_priority)
        return false;

      if(a->group_priority == b->group_priority)
        if(a->internal_priority >= b->internal_priority)
          return false;

      return true;
    }

  };

  class RealInterrupt{
  public:

    RealInterrupt();

    ~RealInterrupt();

    static void init();

    static bool is_up(void);

    static Function* next_int_function(void);

    static void AddInterrupt(StringRef handler_name, uint32_t id, uint32_t group_priority, uint32_t internal_priority);

    static void raise(int id);

  private:

    static std::map<uint32_t, Interrupt*> interrupts_vector;

    static std::priority_queue<Interrupt*, std::vector<Interrupt*>, InterruptComparator> pending_interrupts;

  };
}
#endif
