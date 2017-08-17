#include "inception/Configurator.h"
#include "llvm/ADT/StringRef.h"

#include "llvm/Object/Binary.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/Error.h"
#include "llvm/Object/ObjectFile.h"

#include <map>

using namespace llvm;

namespace Inception {

class SymbolsTable {

public:
~SymbolsTable();

SymbolsTable();

void initTable();

uint64_t lookUp(StringRef name);

private:
void initConfig();

void loadBinary();

std::map<StringRef, uint64_t> symbols;

std::string FileName;

std::unique_ptr<object::ObjectFile> Executable;
};

} // namespace Inception
