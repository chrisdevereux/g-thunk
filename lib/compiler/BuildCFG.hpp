#pragma once

#include "AST.hpp"
#include "CFG.hpp"


namespace compiler {
  // Transform AST into a CFG rooted at `root`.
  cfg::Package buildCFG(ast::Module *module, Arena *arena, Symbol rootName, type::Function const *rootType);
}
