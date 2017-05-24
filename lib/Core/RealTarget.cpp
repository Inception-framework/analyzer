#include "klee/RealTarget.h"

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

    if(RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    switch (w) {
    default:
      assert(0 && "RealTarget called with an invalid width");
    case Expr::Bool:
    case Expr::Int16:
    case Expr::Int64:

      printf(" [ERROR] Unable to perform real read... unexpected size ...\r\n");
      throw std::runtime_error("Unexpected type when redirecting IO to the real target");

    case Expr::Int8:
      *value = jtag_read_u32(RealTarget::inception_device, (uint32_t)address);
      return ConstantExpr::alloc(*value, Expr::Int8);
    case Expr::Int32:

      // printf("Read  at 0x%08x value 0x%08x \r\n", (uint32_t)address, (uint32_t)*value);
      *value = jtag_read_u32(RealTarget::inception_device, (uint32_t)address);
      return ConstantExpr::alloc(*value, Expr::Int32);
    }
  }

  void RealTarget::write(int64_t address, uint64_t value, Expr::Width w){

    if(RealTarget::inception_device == NULL)
      RealTarget::inception_device = jtag_init();

    // printf("Write at 0x%08x value 0x%08x\r\n", (uint32_t)address, (uint32_t)value);

    switch (w) {
    default:
      assert(0 && "invalid width");
    case Expr::Bool:
      printf(" [ERROR] Expr::Bool\r\n");
    case Expr::Int16:
      printf(" [ERROR] Expr::Int16\r\n");
    case Expr::Int64:
      printf(" [ERROR] Expr::Int64\r\n");

      printf(" [ERROR] Unable to perform real read... unexpected size ...\r\n");
      throw std::runtime_error("Unexpected type when redirecting IO to the real target");
    case Expr::Int8:
      jtag_write(RealTarget::inception_device, address, value, 32);
      return;
    case Expr::Int32:
      jtag_write(RealTarget::inception_device, address, value, 32);
      // printf(" [*] Inception performed a write U32");
      return;
    }
  }
}
