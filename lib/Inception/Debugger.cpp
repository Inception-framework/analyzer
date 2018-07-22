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

#include "inception/Debugger.h"

#include <fstream>
#include <iostream>
#include <string>
using namespace std;

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>

#include "klee/ExecutionState.h"
#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;
using namespace klee;

#define INDEX_DUMP_FILE "/home/e3127/Inception/Monitor/index.dump"

namespace Inception {

Debugger::Debugger() {

  active_registers_dump = false;

  portno = 2017;

  loop = 0;

  socklen_t clilen;

  struct sockaddr_in serv_addr, cli_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    klee_error("ERROR opening debugger socket");

  bzero((char *)&serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    klee_error("ERROR on binding debugger socket");

  listen(sockfd, 5);

  clilen = sizeof(cli_addr);

  klee_message("Debugger is waiting for debugger client connection");

  client = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (client < 0)
    klee_error("ERROR on accept debugger client");

  klee_message("Debugger client is connected");

  server_state = WAITING;
}

Debugger::~Debugger() {}

/*
 * Continue must run until we reach another function : call
 */
bool Debugger::evaluateContinue(ExecutionState *state) {

  static StringRef original = current_function;

  if (original.equals("")) {
    original = current_function;
    return true;
  } else if (original.equals(current_function)) {
    return true;
  } else {
    original = "";
    server_state = WAITING;
    return false;
  }
}

/*
 * StepOut must run until we reach the caller function
 */
bool Debugger::evaluateStepOut(ExecutionState *state) {

  if (caller.equals(current_function)) {
    server_state = WAITING;
    loop = 0;
    return false;
  } else {
    return true;
  }
}

bool Debugger::evaluateStep() {
  static char step = 0;

  if (step == 0) {
    step++;
    return true;
  } else {
    step = 0;
    server_state = WAITING;
    return false;
  }
}

bool Debugger::evaluateGoto() {
  if (current_function.equals(StringRef(goto_destination))) {
    server_state = WAITING;
    loop = 0;
    return false;
  } else {
    return true;
  }
}

bool Debugger::isRunning(ExecutionState *state) {

  if (loop == 0 || (server_state != WAITING && server_state != RUNNING &&
                    server_state != STEP_OUT && server_state != GOTO)) {

    std::string str;
    llvm::raw_string_ostream info(str);

    info << *(state->pc->inst);
    this->notify("I\r\n"+info.str());

    loop = 1;
  }

  switch (server_state) {
  case WAITING: // Stop the execution
    loop = 1;
    return false;
    break;

  case RUNNING: // Execute all the instructions
    return true;
    break;

  case CONTINUE: // Continue until next call instruction
    return evaluateContinue(state);
    break;

  case STEP_OUT: // Continue until state return to the caller
    return evaluateStepOut(state);
    break;

  case STEP: // Execute only the next instruction
    return evaluateStep();
    break;
  case STOP:
    stop();
    break;
  case GOTO:
    return evaluateGoto();
    break;
  default:
    return false;
    break;
  }

  return false;
}

void Debugger::run() {

  while (1) {
    char buffer[32] = {0};
    int n = 0;

    if ((n = recv(client, buffer, sizeof buffer - 1, 0)) < 0) {
      klee_error("Unable to receive debugger client command");
    }

    // server_state = (DebuggerState)buffer[0];
    switch (buffer[0]) {
    case '0': // Stop the execution
      server_state = WAITING;
      break;
    case '1': // Execute all the instructions
      server_state = RUNNING;
      break;
    case '2': // Continue until next call instruction
      server_state = CONTINUE;
      break;
    case '3': // Continue until state return to the caller
      server_state = STEP_OUT;
      break;
    case '4': // Execute only the next instruction
      server_state = STEP;
      break;
    case '5':
      server_state = STOP;
      break;
    case '6':
      memset(&goto_destination[0], sizeof goto_destination, 0);
      memcpy(&goto_destination[0], &buffer[1], sizeof buffer - 2);

      // Check if the function exists
      if (module && module->getFunction(StringRef(goto_destination)) != NULL) {
        klee_message("Go to %s", goto_destination);
        server_state = GOTO;
      } else
        server_state = WAITING;
      break;
    case '7':
      active_registers_dump = !(active_registers_dump);
      break;
    default:
      break;
    }

    buffer[n] = '\0';

    memcpy(&arguments[0], &buffer[1], sizeof buffer - 1);
  }
}

void Debugger::updateInfo(ExecutionState *state) {

  if (state->stack.size() > 1 && server_state != STEP_OUT)
    caller = state->stack.at(state->stack.size() - 2).kf->function->getName();

  current_function = state->pc->inst->getParent()->getParent()->getName();

  module = state->pc->inst->getParent()->getParent()->getParent();
}

void Debugger::notify(std::string info) {
  info += "\r\n\r\n";

  if(send(client, info.data(), info.length(), 0) == -1)
  {
    klee_error("Unable to send debug info to client");
  }
}

} // namespace Inception
