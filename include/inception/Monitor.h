/*
 * RealTarget.h
 *
 *  Created on: Jul 12, 2016
 *      Author: Corteggiani Nassim
 */

#ifndef MONITOR_H_
#define MONITOR_H_

#include <iostream>
#include <stdint.h>
#include <string>
#include <map>

#include "llvm/IR/Instructions.h"

#include "klee/Expr.h"
#include "../Core/Executor.h"

using namespace klee;

namespace klee {
  class Executor;
}

namespace Inception {

class Monitor {

  public :

  Monitor();

  ~Monitor();

  static void run();

  static bool running;

  static klee::Executor* executor;

  static std::map<std::string, uint64_t> followed;

  static void follow(const llvm::GlobalVariable* i, uint64_t address);

  static void dump();

  static void init(Executor* _executor);

};

}
#endif
