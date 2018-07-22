/*
    This file is part of Inception-analyzer.

    Inception-analyzer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
*/

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
