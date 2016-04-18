#include "SerializeCFG.hpp"
#include "SerializationTest.hpp"

int main(int argc, char const **argv) {
  return serializationTest(argc, argv, cfg::unserialize::package);
}
