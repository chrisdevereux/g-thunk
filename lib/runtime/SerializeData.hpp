#pragma once

#include "ParseUtil.hpp"
#include "Data.hpp"

namespace vm {
  namespace unserialize {
    parse::Grammar data(parse::GenericAction<Data>);
  }
}

std::ostream &operator<<(std::ostream &str, vm::Data const &value);
