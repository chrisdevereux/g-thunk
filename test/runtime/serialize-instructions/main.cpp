#include "SerializeInstruction.hpp"
#include "SerializationTest.hpp"

int main(int argc, char const **argv) {
  return serializationTest(argc, argv, vm::unserialize::package);
}
