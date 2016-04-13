#pragma once

#include "ParseUtil.hpp"
#include "AST.hpp"

#include <iostream>

namespace ast {
  namespace unserialize {
    // Parse an expression serialized into s-expressions
    parse::Grammar expression(parse::GenericAction<Expression const *> result);
    
    // Parse a module serialized into s-expressions
    parse::Grammar module(parse::GenericAction<Module> result);
  }
  
  namespace serialize {
    // Serialize AST types into s-expressions
    std::ostream &operator<<(std::ostream &str, Expression const &);
    std::ostream &operator<<(std::ostream &str, Module const &);
    std::ostream &operator<<(std::ostream &str, Declaration const &);
  }
}

using namespace ast::serialize;
