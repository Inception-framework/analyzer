/*
 * RealMemory.h
 *
 *  Created on: Jul 12, 2016
 *      Author: Corteggiani Nassim
 */

#ifndef REAL_MEMORY_H_
#define REAL_MEMORY_H_

#include <iostream>
#include <stdint.h>
#include <vector>
#include <string>

namespace Inception {

typedef struct RealAddressSpace {
  uint64_t    base;
  uint64_t    size;
  std::string *name;
} RealAddressSpace;

class RealMemory {

  RealMemory();

  ~RealMemory();

public :

  //Iterate over submemories to check if address is part of any one
  static bool is_real(uint64_t address);

  //Creates a submemory and return it id
  static uint64_t add_submemory(uint64_t base, uint64_t size, std::string *name);

  void delete_all();

  static std::vector<RealAddressSpace*> *submemories;

private :


};

}

#endif
