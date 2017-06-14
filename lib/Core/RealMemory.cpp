#include "klee/RealMemory.h"
#include "klee/Internal/Support/Configurator.h"

#include "stdio.h"

namespace Inception {

std::vector<RealAddressSpace*> *RealMemory::submemories = new std::vector<RealAddressSpace*>();

RealMemory::RealMemory() {}

RealMemory::~RealMemory() {}

void RealMemory::init() {

  ParserMemoryCB callback = &RealMemory::add_submemory;

  while( Configurator::next_memory(callback) == true );

}

bool RealMemory::is_real(uint64_t address) {

  std::vector<RealAddressSpace*>::iterator it;

  uint64_t base, limit;

  // printf(" [*] Inception is comparing concrete memory spaces with address 0x%08x\r\n", (uint32_t)address);

  for (it = RealMemory::submemories->begin(); it != RealMemory::submemories->end(); ++it) {

    base = (*it)->base;

    limit = ( (*it)->base + (*it)->size);

    // printf("    Checking address space [0x%08x:0x%08x]\r\n", (uint32_t)base, (uint32_t)limit);

    if ( address >= base && address < limit ) {
      // printf("[Inception] Access external memory : %s \r\n", (*it)->name->c_str());
      return true;
    }
  }

  return false;
}

void RealMemory::add_submemory(std::string name, uint32_t base, uint32_t size) {

  RealAddressSpace* addr_space = new RealAddressSpace{base, size, name};

  RealMemory::submemories->push_back(addr_space);

  // return RealMemory::submemories->size() - 1;
}

void RealMemory::delete_all() {

  std::vector<RealAddressSpace*>::iterator it;

  for (it = RealMemory::submemories->begin(); it != RealMemory::submemories->end(); ++it) {
    delete *it;
  }
}

}
