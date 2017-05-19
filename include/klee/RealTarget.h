/*
 * RealTarget.h
 *
 *  Created on: Jul 12, 2016
 *      Author: Corteggiani Nassim
 */

#ifndef REAL_TARGET_H_
#define REAL_TARGET_H_

#include <iostream>
#include <stdint.h>
#include <vector>

#include "klee/Expr.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef WATCHER
#define WATCHER
typedef void (*Watcher)(int);
#endif

void *jtag_init(void);

int32_t jtag_read(void *opaque, uint64_t address, uint64_t *value,
                  unsigned size);

uint64_t jtag_read_u32(void *opaque, uint64_t address);

void jtag_write(void *opaque, uint64_t address, uint64_t value, unsigned size);

void benchmark_start(void);

void benchmark_stop(void);

void benchmark_to_string(void);

void benckmark_inc_nread(void);

void benckmark_inc_nwrite(void);

void load_binary_in_sdram(void *opaque, char* file_path, uint32_t address);

void trace_init(void *opaque, Watcher watcher);

#if defined(__cplusplus)
}
#endif


namespace klee {
class Expr;
template<class T> class ref;

}

using namespace klee;

namespace Inception {

class RealTarget {

  public :

  RealTarget();

  ~RealTarget();

  static void* inception_device;

  static ref<Expr> read(uint64_t address, uint64_t *value, Expr::Width w);

  static void write(int64_t address, uint64_t value, Expr::Width w);

};

}
#endif
