#include "BuildCFG.hpp"
#include "SerializeAST.hpp"
#include "SerializeCFG.hpp"
#include "GivenExpectTest.hpp"
#include "GC-CFG.hpp"

int main(int argc, char const *const *argv) {
  Arena arena;
  
  auto vF32 = type::F32()->vectorVersion(&arena);
  Arena::vector<type::Type const *> params(arena.allocator<type::Type const *>());
  params.push_back(vF32);
  
  auto rootType = arena.create<type::Function>(vF32, params);
  auto rootSym = Symbol::get("main");

  return givenExpectTest(argc, argv, ast::unserialize::module, cfg::unserialize::package, [&](ast::Module source) -> cfg::Package {
    
    auto package = compiler::buildCFG(&source, &arena, rootSym, rootType);
    compiler::gcCFG(&arena, &package, {rootType, rootSym});
    
    return package;
  });
}
