#pragma once

#include "CFG.hpp"

namespace compiler {
  // GC a CFG package by performing mark & sweap collection starting
  // at a specific symbol.
  //
  // This isn't required by the compiler, since the AST->CFG
  // transformation implicitly drops unused code. It is useful for
  // test cases, to avoid unused intrinsics being required in the
  // expect clause.
  void gcCFG(Arena *arena, cfg::Package *package, TypedSymbol root);
}
