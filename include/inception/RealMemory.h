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
  std::string name;
} RealAddressSpace;

#ifndef PARSER_CALLBACK
#define PARSER_CALLBACK

typedef void (*ParserInterruptCB)(std::string, uint32_t, uint32_t, uint32_t);

typedef void (*ParserMemoryCB)(std::string, uint32_t, uint32_t);

typedef void (*ParserIrqIDBaseAddrCB)(uint32_t);
#endif

class RealMemory {

  RealMemory();

  ~RealMemory();

public :

  //Iterate over submemories to check if address is part of any one
  static bool is_real(uint64_t address);

  //Creates a submemory and return it id
  static void add_submemory(std::string name, uint32_t base, uint32_t size);

  static void init();

  void delete_all();

  static std::vector<RealAddressSpace*> *submemories;

private :


};

}

#endif
