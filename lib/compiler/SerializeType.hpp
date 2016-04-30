#pragma once

#include "ParseUtil.hpp"
#include "Type.hpp"

namespace type {
  namespace unserialize {
    parse::Grammar type(parse::GenericAction<type::Type const *>);
    parse::Grammar function(parse::GenericAction<type::Function const *>);
  }
}

std::ostream &operator<<(std::ostream &str, type::Type const *type);
