#include "Syntax.hpp"
#include "SerializeAST.hpp"
#include "GivenExpectTest.hpp"

int main(int argc, char const *const *argv) {
  return givenExpectTest(argc, argv, syntax::module, ast::unserialize::module);
}
