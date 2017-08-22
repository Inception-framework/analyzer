/*
 * RealTarget.h
 *
 *  Created on: Jul 12, 2016
 *      Author: Corteggiani Nassim
 */

#ifndef MONITOR_H_
#define MONITOR_H_

#include <iostream>
#include <map>
#include <stdint.h>
#include <string>

#include "llvm/IR/Instructions.h"

#include "../Core/Executor.h"
#include "klee/Expr.h"

using namespace klee;

namespace klee {
class Executor;
}

namespace Inception {

class Monitor {

public:
  Monitor();

  ~Monitor();

  static void run();

  static bool running;

  static klee::Executor *executor;

  static std::map<std::string, uint64_t> followed;

  static void follow(const llvm::GlobalVariable *i, uint64_t address);

  static void dump();

  static void dump_stack(int begin, int end);

  static void init(Executor *_executor);

  static void trace(ExecutionState &state, KInstruction *ki);

  static bool trace_on;

  static bool trace_io_on;

  static void enableInstructionTrace() { trace_on = true; }

  static void disableInstructionTrace() { trace_on = false; }

  static void enableMemTrace() { trace_io_on = true; }

  static void disableMemTrace() { trace_io_on = false; }

  static void traceIO(ref<Expr> address, ref<Expr> value, bool isWrite,
                      KInstruction *target);
};

} // namespace Inception
#endif
