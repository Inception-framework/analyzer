#include "string"
#include <fstream>
#include <iostream>
#include <sstream>

#include <json/json.h>

#include "inception/Configurator.h"
#include "klee/Internal/Support/ErrorHandling.h"

using namespace std;
using namespace klee;

namespace Inception {

std::string Configurator::file_name = "config.json";

Json::Value *Configurator::root = NULL;

unsigned int Configurator::interrupt_index = 0;

unsigned int Configurator::memory_index = 0;

bool Configurator::file_present = false;

Configurator::Configurator() {}

Configurator::~Configurator() {}

void Configurator::init() {

  if (Configurator::root == NULL) {

    std::ifstream config_file(file_name, std::ifstream::binary);
    if (config_file) {

      Json::Value data;

      Configurator::file_present = true;

      Configurator::root = new Json::Value();

      config_file >> (*Configurator::root);
    } else
      klee_error("Configuration file not found !");
  }
}

std::string Configurator::getAsString(std::string section, std::string category,
                                      uint32_t line) {

  Configurator::init();

  if (Configurator::file_present == false)
    return NULL;

  const Json::Value Section = (*Configurator::root)[section];

  return Section[line].get(category, "").asString();
}

uint32_t Configurator::getAsInteger(std::string section, std::string category,
                                    uint32_t line) {

  Configurator::init();

  if (Configurator::file_present == false)
    return 0;

  const Json::Value Section = (*Configurator::root)["Stub"];

  uint32_t address;

  std::stringstream ss;

  std::string s_address = Section[line].get("address", "0").asString();
  ss << std::hex << s_address;
  ss >> address;
  ss.clear();

  return address;
}

bool Configurator::next_memory(SymbolsTable *sy) {

  Configurator::init();

  if (Configurator::file_present == false)
    return false;

  const Json::Value realMemory = (*Configurator::root)["RealMemory"];

  uint32_t address, size;
  bool symbolic;

  std::stringstream ss;

  // Iterate over the sequence elements.
  if (Configurator::memory_index < realMemory.size()) {

    std::string name =
        realMemory[Configurator::memory_index].get("name", "0").asString();

    std::string s_address =
        realMemory[Configurator::memory_index].get("address", "0").asString();
    ss << std::hex << s_address;
    ss >> address;
    ss.clear();

    std::string s_size =
        realMemory[Configurator::memory_index].get("size", "0").asString();
    ss << std::hex << s_size;
    ss >> size;
    ss.clear();

    symbolic = false;
        // realMemory[Configurator::memory_index].get("symbolic", false).asBool();

    char* dst = new char[name.length()+1];
    name.copy(dst, name.length());
    dst[name.length()]='\0';

    // (sy->*callback)(llvm::StringRef(name), address, size, symbolic);
    sy->addSymbol(llvm::StringRef(dst, name.length()), address, size, symbolic);

    Configurator::memory_index++;

    return true;
  } else
    Configurator::file_present == false;

  return false;
}

bool Configurator::next_interrupt(ParserInterruptCB callback) {

  Configurator::init();

  if (Configurator::file_present == false)
    return false;

  const Json::Value realInterrupt = (*Configurator::root)["RealInterrupt"];

  uint32_t priority_g, priority, id;

  std::stringstream ss;

  // Iterate over the sequence elements.
  if (Configurator::interrupt_index < realInterrupt.size()) {

    std::string name = realInterrupt[Configurator::interrupt_index]
                           .get("name", "0")
                           .asString();

    std::string s_priority_g = realInterrupt[Configurator::interrupt_index]
                                   .get("priority_g", "0")
                                   .asString();
    ss << s_priority_g;
    ss >> priority_g;
    ss.clear();

    std::string s_priority = realInterrupt[Configurator::interrupt_index]
                                 .get("priority", "0")
                                 .asString();
    ss << s_priority;
    ss >> priority;
    ss.clear();

    std::string s_id =
        realInterrupt[Configurator::interrupt_index].get("id", "0").asString();
    ss << s_id;
    ss >> id;
    ss.clear();

    std::string handler = realInterrupt[Configurator::interrupt_index]
                              .get("handler", "0")
                              .asString();

    callback(handler, id, priority_g, priority);

    Configurator::interrupt_index++;

    return true;
  } else
    Configurator::file_present == false;

  return false;
}

} // namespace Inception
