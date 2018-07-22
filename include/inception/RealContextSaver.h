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

#ifndef REAL_CONTEXT_SAVER_H
#define REAL_CONTEXT_SAVER_H

#include "klee/Internal/Module/KInstIterator.h"

namespace klee{
  class KInstIterator;
}

namespace Inception{

  class RealContextSaver{
  public:

    RealContextSaver();

    ~RealContextSaver();

    static void save(klee::KInstIterator &next_pc, klee::KInstIterator &current_pc);
  };
}

#endif
