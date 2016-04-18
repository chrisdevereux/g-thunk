#pragma once

#include "ParseUtil.hpp"
#include "Instruction.hpp"

#include <iostream>

namespace vm {
  namespace unserialize {
    parse::Grammar package(parse::GenericAction<Package>);
  }
}

std::ostream &operator <<(std::ostream &str, vm::Instruction const &inst);
std::ostream &operator <<(std::ostream &str, vm::Package const &inst);
