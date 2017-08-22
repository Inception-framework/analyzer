#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H
#include <json/json.h>

#include "inception/SymbolsTable.h"

namespace Inception {

#ifndef PARSER_CALLBACK
#define PARSER_CALLBACK

typedef void (*ParserInterruptCB)(std::string, uint32_t, uint32_t, uint32_t);
#endif

class Configurator {

public:
  Configurator();
  ~Configurator();

  static void init();

  static std::string file_name;

  static Json::Value *root;

  static bool next_interrupt(ParserInterruptCB callback);

  static unsigned int interrupt_index;

  static bool next_memory(SymbolsTable *sy);

  static unsigned int memory_index;

  static bool file_present;

  static std::string getAsString(std::string section, std::string category,
                                 uint32_t line);

  static uint32_t getAsInteger(std::string section, std::string category,
                               uint32_t line);
};
}

#endif
