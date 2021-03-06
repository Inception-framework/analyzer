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

#include "inception/RealTarget.h"

#include "stdio.h"
#include "klee/Expr.h"

#include <string>
#include <exception>
#include <stdexcept>


namespace Inception {

  void* RealTarget::inception_device = NULL;

  RealTarget::RealTarget(){}

  RealTarget::~RealTarget(){}

  ref<Expr> RealTarget::read(uint64_t address, uint64_t *value, Expr::Width w){

    uint32_t b_address = 0;

    if(RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    switch (w) {
    default:
      assert(0 && "RealTarget called with an invalid width");
    case Expr::Bool:
    case Expr::Int8:

      b_address = address - (address % 4);

      *value = jtag_read_u32(RealTarget::inception_device, (uint32_t)b_address);
      switch((address % 4)) {
        case 3:
          *value = (*value & 0xFF000000) >> 24;
          break;
        case 2:
          *value = (*value & 0x00FF0000) >> 16;
          break;
        case 1:
          *value = (*value & 0x0000FF00) >> 8;
          break;
        case 0:
          *value = (*value & 0x000000FF);
          break;
      }

      return ConstantExpr::alloc(*value, Expr::Int8);
    case Expr::Int16:

      b_address = address - (address % 4);

      *value = jtag_read_u32(RealTarget::inception_device, b_address);
      if( address % 4 == 0 )
        *value &= 0x0000FFFF;
      else
        *value = (*value & 0xFFFF0000) >> 16;

      return ConstantExpr::alloc(*value, Expr::Int16);

    case Expr::Int32:

      // printf("Read  at 0x%08x value 0x%08x \r\n", (uint32_t)address, (uint32_t)*value);
      *value = jtag_read_u32(RealTarget::inception_device, (uint32_t)address);
      return ConstantExpr::alloc(*value, Expr::Int32);

    case Expr::Int64:

      printf(" [ERROR] Unable to perform real read... unexpected size ...\r\n");
      throw std::runtime_error("Unexpected type when redirecting IO to the real target");

    }
  }

  void RealTarget::write(int64_t address, uint64_t value, Expr::Width w){

    uint32_t new_val = 0, b_address = 0;

    if(RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    // printf("Write at 0x%08x value 0x%08x\r\n", (uint32_t)address, (uint32_t)value);

    switch (w) {
    default:
      assert(0 && "invalid width");
    case Expr::Bool:
    case Expr::Int8:

      b_address = address - (address % 4);

      new_val = jtag_read_u32(RealTarget::inception_device, b_address);

      //Is the access memory alligned
      switch((address % 4)) {
        case 3:
          new_val = (new_val & 0x00FFFFFF) | (value << 24);
          break;
        case 2:
          new_val = (new_val & 0xFF00FFFF) | ((value & 0x000000FF) << 16 );
          break;
        case 1:
          new_val = (new_val & 0xFFFF00FF) | ((value & 0x000000FF) << 8 );
          break;
        case 0:
          new_val = (new_val & 0xFFFFFF00) | (value & 0x000000FF);
          break;
      }

      jtag_write(RealTarget::inception_device, b_address, new_val, 32);
      return;
    case Expr::Int16:

      b_address = address - (address % 4);

      new_val = jtag_read_u32(RealTarget::inception_device, b_address);
      //Is the access memory alligned
      if( address % 4 == 0 )
        new_val = (new_val & 0xFFFF0000) | (value & 0x0000FFFF);
      else
        new_val = (new_val & 0x0000FFFF) | (value << 16);

      jtag_write(RealTarget::inception_device, b_address, new_val, 32);
      return;
    case Expr::Int64:
      printf(" [ERROR] Expr::Int64\r\n");

      printf(" [ERROR] Unable to perform real read... unexpected size ...\r\n");
      throw std::runtime_error("Unexpected type when redirecting IO to the real target");
    case Expr::Int32:
      jtag_write(RealTarget::inception_device, address, value, 32);
      // printf(" [*] Inception performed a write U32");
      return;
    }
  }
}
