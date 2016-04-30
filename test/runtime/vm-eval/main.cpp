#include "VMEval.hpp"
#include "SerializeInstruction.hpp"
#include "SerializeData.hpp"
#include "EvalTest.hpp"

int main(int argc, char const *const *argv) {
  using vm::unserialize::package;
  using vm::unserialize::data;
  
  return evalTest(argc, argv, package, data, data, [](vm::Package package, vm::Data const &params) {
    return vm::eval(&package, Symbol::get("main"), params);
  });
}
