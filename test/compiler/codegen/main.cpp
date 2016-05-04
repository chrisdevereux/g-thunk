#include "SerializeInstruction.hpp"
#include "SerializeCFG.hpp"
#include "Codegen.hpp"
#include "GivenExpectTest.hpp"

int main(int argc, char const *const *argv) {
  Arena arena;
  
  return givenExpectTest(argc, argv, cfg::unserialize::package, vm::unserialize::package, [&](cfg::Package source) -> vm::Package {
    return compiler::codegen(&source, &arena);
  });
}
