#pragma once

#include "ParseUtil.hpp"
#include "AST.hpp"

namespace syntax {
  parse::Grammar expression(parse::GenericAction<ast::Expression *>);
  parse::Grammar module(parse::GenericAction<ast::Module>);
}
