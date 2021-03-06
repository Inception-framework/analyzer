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

void jtag_halt(void *opaque);

void jtag_resume(void *opaque);

int32_t jtag_read(void *opaque, uint64_t address, uint64_t *value,
                  unsigned size);

uint64_t jtag_read_u32(void *opaque, uint64_t address);

void jtag_write(void *opaque, uint64_t address, uint64_t value, unsigned size);

void jtag_write_reg(void *opaque, uint32_t reg_id, uint32_t value);

uint32_t jtag_read_reg(void *opaque, uint32_t reg_id);

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

  static void halt();

  static void resume();

  static ref<Expr> read(uint64_t address, uint64_t *value, Expr::Width w);

  static void write(int64_t address, uint64_t value, Expr::Width w);

};

}
#endif
