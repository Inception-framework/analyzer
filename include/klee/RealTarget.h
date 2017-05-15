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

extern void *jtag_init(void);

extern int32_t jtag_read(void *opaque, uint64_t address, uint64_t *value,
  unsigned size);

extern uint64_t jtag_read_u32(void *opaque, uint64_t address);

extern void jtag_write(void *opaque, uint64_t address, uint64_t value, unsigned size);

extern void benchmark_start(void);

extern void benchmark_stop(void);

extern void benchmark_to_string(void);

extern void benckmark_inc_nread(void);

extern void benckmark_inc_nwrite(void);

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

  RealTarget();

  ~RealTarget();

  static void* inception_device;

public :

  static ref<Expr> read(uint64_t address, uint64_t *value, Expr::Width w);

  static void write(int64_t address, uint64_t value, Expr::Width w);

};

}
#endif
