#include "inception/SymbolsTable.h"

#include "inception/Configurator.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/ADT/StringRef.h"

using namespace llvm;
using namespace klee;

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

  fillGaps(mod);
}

SymbolsTable::~SymbolsTable() {}

void SymbolsTable::initExtern(llvm::Module *mod) {

  std::map<StringRef, SymbolInfo *>::iterator it;

  for (it = symbols.begin(); it != symbols.end(); ++it) {

    SymbolInfo *I = it->second;

    // Set initializer
    std::vector<Constant *> const_init_array;
    for (int i = 0; i < I->size; i++) {
      const_init_array.push_back(
          ConstantInt::get(mod->getContext(), APInt(8, I->initializer, 16)));
    }

    ArrayType *Ty =
        ArrayType::get(IntegerType::get(mod->getContext(), 8), I->size);

    // Constant *Initializer = Constant::getNullValue(Ty);
    Constant *Initializer = ConstantArray::get(Ty, const_init_array);

    // printf("\tSymbol %s [0x%lx:0x%lx]\r\n", it->first.str().c_str(), I->base,
    // I->size);

    new GlobalVariable(*mod,  // Module
                       Ty,    // Type
                       false, // isConstant
                       GlobalValue::CommonLinkage, Initializer, it->first);

    // g->dump();
  }
}

void SymbolsTable::fillGaps(llvm::Module *mod) {

  // Retrieve .data section information
  SymbolInfo *section_info = lookUpSection(".data");
  if (section_info == NULL) {
    klee::klee_warning("[SymbolsTable] No .data section found");
    return;
  }

  uint64_t section_size = section_info->size;
  uint64_t section_base = section_info->base;
  uint64_t section_end = section_base + section_size;

  SymbolInfo *symbole;

  // For each valid address of section_info we need to check if this address
  // is owned by an LLVM IR variables. If no, we allocate a new IR var there.
  for (uint64_t i = section_base; i < section_end; i++) {
    // Is there IR variables owning address i ? NULL if no.
    if ((symbole = isAddressOwned(i, mod)) == NULL) {

      // Get next owned address
      uint64_t next = getNextGVar(i, mod);
      if (next == 0)
        next = section_end;

      uint64_t size = (next - i);
      size = size == 0 ? 1 : size;

      Type *Ty;
      Ty = ArrayType::get(IntegerType::get(mod->getContext(), 32), size);
      Constant *Initializer = Constant::getNullValue(Ty);

      std::string *name = new std::string("inception_gap_" + std::to_string(i));

      GlobalVariable *data = new GlobalVariable(
          /*Module=*/*mod,
          /*Type=*/Ty,
          /*isConstant=*/false,
          /*Linkage=*/GlobalValue::CommonLinkage,
          /*Initializer=*/Initializer, // has initializer, specified below
          /*Name=*/StringRef(name->c_str()));

      addSymbol(StringRef(name->c_str()), i, size, false, false, 0);

      i += size;
    }
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

    SymbolInfo *Info =
        new SymbolInfo(SymName, SymAddr, SymSize, false, false, 0);

    // printf("\tSymbol %s at 0x%08x of 0x%08x B\n", SymName.str().c_str(),
    //       SymAddr, SymSize);
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
    SymbolInfo *Info =
        new SymbolInfo(SymName, SymAddr, SymSize, false, false, 0);

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
