#include "inception/SymbolsTable.h"

#include "inception/Configurator.h"
#include "llvm/ADT/StringRef.h"

using namespace llvm;

namespace Inception {

SymbolsTable::SymbolsTable() {
  // Load the configuration file
  initConfig();

  // Load binary object
  loadBinary();

  initTable();
}

SymbolsTable::~SymbolsTable() {}

void SymbolsTable::initTable() {
  uint64_t SymAddr, SymSize;
  StringRef SymName;
  std::error_code ec;

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

    printf("\tSymbol %s at 0x%08x of 0x%08x B\n", SymName.str().c_str(),
           SymAddr, SymSize);
    symbols.insert(std::pair<StringRef, std::pair<uint64_t, uint64_t> >(
        SymName, std::pair<uint64_t, uint64_t>(SymAddr, SymSize)));
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

    printf("\tSections %s at 0x%08x of 0x%08x B\n", SymName.str().c_str(),
           SymAddr, SymSize);
    sections.insert(std::pair<StringRef, std::pair<uint64_t, uint64_t> >(
        SymName, std::pair<uint64_t, uint64_t>(SymAddr, SymSize)));
  }

}

std::pair<uint64_t, uint64_t> SymbolsTable::lookUpVariable(StringRef name) {

  std::map<StringRef, std::pair<uint64_t, uint64_t>>::iterator it;

  it = symbols.find(name);
  if (it != symbols.end()) {
    printf("[SymbolsTable]\n");
    printf("\t%s Located at 0x%08x\n", name.str().c_str(), it->second);
    return it->second;
  }

  return std::pair<uint64_t, uint64_t>(0, 0);
}

std::pair<uint64_t, uint64_t> SymbolsTable::lookUpSection(StringRef name) {

  std::map<StringRef, std::pair<uint64_t, uint64_t>>::iterator it;

  it = sections.find(name);
  if (it != sections.end()) {
    printf("[SymbolsTable]\n");
    printf("\t%s Located at 0x%08x\n", name.str().c_str(), it->second);
    return it->second;
  }

  return std::pair<uint64_t, uint64_t>(0, 0);
}

void SymbolsTable::initConfig() {

  FileName = Configurator::getAsString("Binary", "Path", 0);
}

void SymbolsTable::loadBinary() {

  if (FileName != "-" && !sys::fs::exists(FileName)) {
    errs() << "[SymbolsTable] No such binary file or directory: '"
           << FileName.data() << "'.\n";
    throw std::runtime_error("[SymbolsTable] No such binary file or directory");
  }

  ErrorOr<object::OwningBinary<object::Binary> > Binary =
      object::createBinary(FileName);
  if (std::error_code err = Binary.getError()) {
    errs() << "[SymbolsTable] Unknown binary file format: '" << FileName.data()
           << "'.\n Error Msg: " << err.message() << "\n";

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
