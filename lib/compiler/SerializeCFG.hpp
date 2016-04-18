#pragma once

#include "ParseUtil.hpp"
#include "Symbol.hpp"
#include "CFG.hpp"

namespace cfg {
  namespace unserialize {
    parse::Grammar value(parse::GenericAction<Value *>);
    parse::Grammar package(parse::GenericAction<Package>);
  }
}

std::ostream &operator <<(std::ostream &str, const cfg::Value &fn);
std::ostream &operator <<(std::ostream &str, const cfg::Package &fn);
