/*
    This file is part of Inception analyzer.

    Inception-analyzer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Inception analyzer.  If not, see <https://www.gnu.org/licenses/>.

    Copyright (c) 2017 Maxim Integrated, Inc.
    Author: Nassim Corteggiani <nassim.corteggiani@maximintegrated.com>

    Copyright (c) 2017 EURECOM, Inc.
    Author: Giovanni Camurati <giovanni.camurati@eurecom.fr>
*/

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

  static void AddInterrupt(std::string handler_name, uint32_t id,
                           uint32_t group_priority, uint32_t internal_priority);

  static void raise(int id);

  static void stop_interrupt();

  static llvm::Function *caller;

  static Interrupt *current_interrupt;

  static klee::Executor *executor;

  static bool enabled;

  static void disable();

  static void enable();

  static void is_irq(klee::ref<klee::Expr> interrupted_ptr);

  static void serve_pending_interrupt();

  static void write_basepri(klee::ref<klee::Expr> basepri);

  static void read_basepri(klee::ref<klee::Expr> basepri_ptr);

  static bool isDeviceConnected;

private:
  static uint32_t interrupt_vector_base_addr;

  static uint32_t irq_id_base_addr;

  static uint32_t stub_ack_address;

  static std::map<uint32_t, Interrupt *> interrupts_vector;

  static std::priority_queue<Interrupt *, std::vector<Interrupt *>,
                             InterruptComparator>
      pending_interrupts;

  static bool isDynamicInterruptTable;
};
} // namespace Inception
#endif
