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
  SymbolInfo(llvm::StringRef _name, uint64_t _base, uint64_t _size, bool _symbolic,
             bool _redirected) {
    name = _name;
    base = _base;
    size = _size;
    symbolic = _symbolic;
    redirected = _redirected;
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

  void addSymbol(llvm::StringRef name, uint32_t base, uint32_t size, bool symolic) {

    SymbolInfo *Info = new SymbolInfo(name, base, size, symolic, true);

    printf("Adding symbol(%s, 0x%08x, 0x%08x)\n", name.str().c_str(), Info->base,
           Info->size);

    symbols.insert(std::pair<StringRef, SymbolInfo *>(name, Info));
  }

  void dump() {
    std::map<StringRef, SymbolInfo *>::iterator it;

    uint64_t base, limit;

    for (it = symbols.begin(); it != symbols.end(); ++it) {

      SymbolInfo *I = it->second;

      printf("\Symbol %s [0x%lx:0x%lx]\r\n", it->first.str().c_str(), I->base, I->size);
    }
    printf("------------------------------\n");
  }

private:
  void initConfig();

  void loadBinary();

  std::map<StringRef, SymbolInfo *> symbols;

  std::map<StringRef, SymbolInfo *> sections;

  std::string FileName;

  std::unique_ptr<object::ObjectFile> Executable;

  bool disable;
};

} // namespace Inception
#endif
