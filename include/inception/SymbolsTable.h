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

#include "inception/Configurator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"

#include <map>

using namespace llvm;

#ifndef SYMBOLS_TABME_H
#define SYMBOLS_TABME_H

namespace Inception {

typedef struct SymbolInfo {
  uint64_t base;
  uint64_t size;
  llvm::StringRef name;
  bool symbolic;
  bool redirected;
  uint64_t initializer;
  SymbolInfo(llvm::StringRef _name, uint64_t _base, uint64_t _size,
             bool _symbolic, bool _redirected, uint64_t _initializer) {
    name = _name;
    base = _base;
    size = _size;
    symbolic = _symbolic;
    redirected = _redirected;
    initializer = _initializer;
  }
} SymbolInfo;

class SymbolsTable {

public:
  ~SymbolsTable();

  SymbolsTable(llvm::Module *mod);

  void initExtern(llvm::Module *mod);

  void initTable();

  SymbolInfo *lookUpVariable(StringRef name);

  SymbolInfo *lookUpSection(StringRef name);

  void addSymbol(llvm::StringRef name, uint32_t base, uint32_t size,
                 bool symbolic, bool redirected = true,
                 uint64_t initializer = 0) {

    SymbolInfo *Info =
        new SymbolInfo(name, base, size, symbolic, redirected, initializer);

    // printf("Adding symbol(%s, 0x%08x, 0x%08x, 0x%08x)\n", name.str().c_str(),
    //        Info->base, Info->size, initializer);

    symbols.insert(std::pair<StringRef, SymbolInfo *>(name, Info));
  }

  void dump() {
    std::map<StringRef, SymbolInfo *>::iterator it;

    // uint64_t base, limit;

    for (it = symbols.begin(); it != symbols.end(); ++it) {

      SymbolInfo *I = it->second;

      printf("Symbol %s [0x%lx:0x%lx]\r\n", it->first.str().c_str(), I->base,
             I->size);
    }
    printf("------------------------------\n");
  }

private:
  void initConfig();

  void loadBinary();

  void fillGaps(llvm::Module *mod);

  std::map<StringRef, SymbolInfo *> symbols;

  std::map<StringRef, SymbolInfo *> sections;

  std::string FileName;

  std::unique_ptr<object::ObjectFile> Executable;

  bool disable;

  SymbolInfo *isAddressOwned(uint64_t i, llvm::Module *mod) {
    for (std::map<StringRef, SymbolInfo *>::iterator si = symbols.begin(),
                                                     se = symbols.end();
         si != se; ++si) {
      uint64_t address = si->second->base;
      uint64_t size = si->second->size;

      if (size == 0)
        size = 1;

      if (i >= address && i < (address + size)) {
        if (mod->getGlobalVariable(si->second->name) != NULL) {
          return si->second;
        }
      }
    }
    return NULL;
  }

  uint64_t getNextGVar(uint64_t i, llvm::Module *mod) {
    uint64_t min = 0;

    for (std::map<StringRef, SymbolInfo *>::iterator si = symbols.begin(),
                                                     se = symbols.end();
         si != se; ++si) {
      uint64_t address = si->second->base;
      uint64_t size = si->second->size;
      if (size == 0)
        size = 1;

      if (mod->getGlobalVariable(si->second->name) != NULL) {
        if (i <= address) {
          if (min == 0 || min > address)
            min = address;
        }
      }
    }
    return min;
  }
};

} // namespace Inception
#endif
