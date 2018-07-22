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

  static std::string dump();

  static std::string dump(llvm::Function* function, llvm::Instruction* inst);

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
