#include "TypedSymbol.hpp"
#include "SerializeType.hpp"

std::ostream &operator<<(std::ostream &str, TypedSymbol const &sym) {
  return str << sym.name << "_" << sym.type;
}
