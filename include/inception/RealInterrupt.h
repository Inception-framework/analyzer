#ifndef REAL_INTERRUPT_H
#define REAL_INTERRUPT_H

// #include "klee/Internal/Module/KInstIterator.h"

#include "stdlib.h"

// #include "llvm/Function.h"
#include "klee/Internal/ADT/RNG.h"
#include "klee/Internal/Module/KModule.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"

#include "list"
#include "map"
#include <iostream>
#include <queue>

using namespace llvm;

namespace klee {
class KInstIterator;
class ExecutionState;
class Executor;
} // namespace klee

namespace Inception {

#ifndef PARSER_CALLBACK
#define PARSER_CALLBACK

typedef void (*ParserInterruptCB)(std::string, uint32_t, uint32_t, uint32_t);

typedef void (*ParserMemoryCB)(std::string, uint32_t, uint32_t);

#endif

class Interrupt {

public:
  Interrupt(std::string _handlerName, uint32_t _id, uint32_t _group_priority,
            uint32_t _internal_priority)
      : handlerName(_handlerName), id(_id), group_priority(_group_priority),
        internal_priority(_internal_priority), state(0) {}

  Interrupt();

  ~Interrupt();

  std::string handlerName;

  uint32_t id;

  uint32_t group_priority;

  uint32_t internal_priority;

  klee::ExecutionState *state;
};

class InterruptComparator {
public:
  bool operator()(Interrupt *a, Interrupt *b) {
    if (a->group_priority > b->group_priority)
      return false;

    if (a->group_priority == b->group_priority)
      if (a->internal_priority >= b->internal_priority)
        return false;

    return true;
  }
};

class RealInterrupt {
public:
  RealInterrupt();

  ~RealInterrupt();

  static bool interrupted;

  static void init(klee::Executor *_executor);

  static bool is_up(void);

  static void AddInterrupt(std::string handler_name, uint32_t id,
                           uint32_t group_priority, uint32_t internal_priority);

  static void raise(int id);

  static bool is_interrupted();

  static void stop_interrupt();

  static klee::ExecutionState *next_without_priority();

  static klee::ExecutionState *next();

  static bool masked();

  static klee::ExecutionState *create_interrupt_state();

  static klee::ExecutionState *interrupt_state;

  static llvm::Function *caller;

  static bool ending;

  static Interrupt *current_interrupt;

  static klee::Executor *executor;

  static klee::ExecutionState *getPending();

  static bool enabled;

  static void disable();

  static void enable();

  static klee::RNG *random;

  static uint32_t stub_ack_address;

private:
  static std::map<uint32_t, Interrupt *> interrupts_vector;

  static std::priority_queue<Interrupt *, std::vector<Interrupt *>,
                             InterruptComparator>
      pending_interrupts;
};
}
#endif
