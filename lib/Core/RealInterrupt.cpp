#include "klee/RealInterrupt.h"

namespace Inception{

RealInterrupt::RealInterrupt(){}

RealInterrupt::~RealInterrupt(){}

bool RealInterrupt::is_up() {
  return false;
}

Function* RealInterrupt::next_int_function() {
  return NULL;
}

}
