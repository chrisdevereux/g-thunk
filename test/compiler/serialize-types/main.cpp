#include "SerializeType.hpp"
#include "SerializationTest.hpp"
#include "Util.hpp"

int main(int argc, char const **argv) {
  return serializationTest(argc, argv, type::unserialize::type, equalData<type::Type>);
}
