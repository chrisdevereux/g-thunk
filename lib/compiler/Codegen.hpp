#pragma once

#include "Arena.hpp"
#include "Instruction.hpp"
#include "CFG.hpp"

namespace compiler {
  // cfg::Package -> vm::Package tranformation.
  // Converts program CFG into flat array of bytecode + symbol table.
  vm::Package codegen(cfg::Package const *sources, Arena *arena);
}
