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

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
using namespace llvm;

#include "klee/Internal/Support/ErrorHandling.h"

namespace klee {
class ExecutionState;
}

namespace Inception {

typedef enum {
  WAITING = 0,  // Stop the execution
  RUNNING = 1,  // Execute all the instructions
  CONTINUE = 2, // Continue until next call instruction
  STEP_OUT = 3, // Continue until state return to the caller
  STEP = 4,     // Execute only the next instruction
  STOP = 5,    // Close the server and shutdown klee
  GOTO = 6,
} DebuggerState;

class Debugger {

public:
  Debugger();

  ~Debugger();

  bool isRunning(klee::ExecutionState *state);

  void run();

  void updateInfo(klee::ExecutionState *state);

  bool active_registers_dump;

  void notify(std::string info);

private:
  DebuggerState server_state;

  int sockfd;

  int client;

  int portno;

  char arguments[32];

  llvm::StringRef current_function;

  llvm::StringRef caller;

  char goto_destination[32];

  llvm::Module* module;

  bool evaluateContinue(klee::ExecutionState *state);

  bool evaluateStepOut(klee::ExecutionState *state);

  bool evaluateStep();

  bool evaluateGoto();

  void stop() {
    close(sockfd);
    close(client);
    klee::klee_error("Debugger closed the session");
  }

  unsigned loop;
};

} // namespace Inception
#endif
