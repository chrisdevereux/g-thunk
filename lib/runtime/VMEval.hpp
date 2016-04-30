#pragma once

#include "Symbol.hpp"
#include "Data.hpp"

namespace vm {
  struct Package;
  
  Data eval(Package *package, Symbol symbol, Data const &param, size_t stackSize = 16 * 1024);
}
