#include "inception/SymbolsTable.h"

#include "inception/Configurator.h"
#include "llvm/ADT/StringRef.h"

using namespace llvm;

namespace Inception {

SymbolsTable::SymbolsTable(llvm::Module *mod) {

  disable = false;

  // Load the configuration file
  initConfig();

  while (Configurator::next_memory(this) == true)
    ;

  // dump();

  initExtern(mod);

  // Load binary object
  loadBinary();

  initTable();
}

SymbolsTable::~SymbolsTable() {}

void SymbolsTable::initExtern(llvm::Module *mod) {

  std::map<StringRef, SymbolInfo *>::iterator it;

  for (it = symbols.begin(); it != symbols.end(); ++it) {

    SymbolInfo *I = it->second;

    Type *Ty = ArrayType::get(IntegerType::get(mod->getContext(), 8), I->size);

    Constant *Initializer = Constant::getNullValue(Ty);

    // printf("\tSymbol %s [0x%lx:0x%lx]\r\n", it->first.str().c_str(), I->base,
    // I->size);

    new GlobalVariable(*mod,  // Module
                       Ty,    // Type
                       false, // isConstant
                       GlobalValue::CommonLinkage, Initializer, it->first);

    // v->dump();
  }
}

void SymbolsTable::initTable() {
  uint64_t SymAddr, SymSize;
  StringRef SymName;
  std::error_code ec;

  if (disable)
    return;

  for (object::symbol_iterator I = Executable->symbols().begin(),
                               E = Executable->symbols().end();
       I != E; ++I) {

    if ((ec = I->getName(SymName))) {
      errs() << ec.message() << "\n";
      continue;
    }

    if ((ec = I->getAddress(SymAddr))) {
      errs() << ec.message() << "\n";
      continue;
    }

    if ((ec = I->getSize(SymSize))) {
      errs() << ec.message() << "\n";
      continue;
    }

    SymbolInfo *Info = new SymbolInfo(SymName, SymAddr, SymSize, false, false);

    printf("\tSymbol %s at 0x%08x of 0x%08x B\n", SymName.str().c_str(),
           SymAddr, SymSize);
    symbols.insert(std::pair<StringRef, SymbolInfo *>(SymName, Info));
  }

  for (object::section_iterator I = Executable->sections().begin(),
                                E = Executable->sections().end();
       I != E; ++I) {

    if ((ec = I->getName(SymName))) {
      errs() << ec.message() << "\n";
      continue;
    }

    SymAddr = I->getAddress();

    SymSize = I->getSize();

    // printf("\tSection %s at 0x%08x of 0x%08x B\n", SymName.str().c_str(),
    //        SymAddr, SymSize);
    SymbolInfo *Info = new SymbolInfo(SymName, SymAddr, SymSize, false, false);

    sections.insert(std::pair<StringRef, SymbolInfo *>(SymName, Info));
  }

  // dump();
}

SymbolInfo *SymbolsTable::lookUpVariable(StringRef name) {

  std::map<StringRef, SymbolInfo *>::iterator it;

  it = symbols.find(name);
  if (it != symbols.end()) {
    // printf("[SymbolsTable]\n");
    // printf("\t%s Located at 0x%lx\n", name.str().c_str(), it->second->base);
    return it->second;
  }

  // printf("[SymbolsTable]\n\t Did not find symbol %s\n", name.str().c_str());
  return NULL;
}

SymbolInfo *SymbolsTable::lookUpSection(StringRef name) {

  std::map<StringRef, SymbolInfo *>::iterator it;

  it = sections.find(name);
  if (it != sections.end()) {
    // printf("[SymbolsTable]\n");
    // printf("\t%s Located at 0x%lx\n", name.str().c_str(), it->second->base);
    return it->second;
  }

  return NULL;
}

void SymbolsTable::initConfig() {

  FileName = Configurator::getAsString("Binary", "Path", 0);
}

void SymbolsTable::loadBinary() {

  if (FileName != "-" && !sys::fs::exists(FileName)) {
    errs() << "[SymbolsTable] No such binary file or directory: '"
           << FileName.data() << "'.\n";
    disable = true;
    return;
    throw std::runtime_error("[SymbolsTable] No such binary file or directory");
  }

  ErrorOr<object::OwningBinary<object::Binary> > Binary =
      object::createBinary(FileName);
  if (std::error_code err = Binary.getError()) {
    errs() << "[SymbolsTable] Unknown binary file format: '" << FileName.data()
           << "'.\n Error Msg: " << err.message() << "\n";
    disable = true;
    return;
    throw std::runtime_error("[SymbolsTable] Unknown binary file format");
  } else {
    if (Binary.get().getBinary()->isObject()) {
      std::pair<std::unique_ptr<object::Binary>, std::unique_ptr<MemoryBuffer> >
          res = Binary.get().takeBinary();
      ErrorOr<std::unique_ptr<object::ObjectFile> > ret =
          object::ObjectFile::createObjectFile(
              res.second.release()->getMemBufferRef());
      Executable.swap(ret.get());
    }
  }
}
} // namespace Inception
