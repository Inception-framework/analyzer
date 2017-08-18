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

std::pair<uint64_t, uint64_t> lookUpVariable(StringRef name);

std::pair<uint64_t, uint64_t> lookUpSection(StringRef name);

private:
void initConfig();

void loadBinary();

std::map<StringRef, std::pair<uint64_t, uint64_t>> symbols;

std::map<StringRef, std::pair<uint64_t, uint64_t>> sections;

std::string FileName;

std::unique_ptr<object::ObjectFile> Executable;
};

} // namespace Inception
