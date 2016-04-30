#pragma once

#include "Type.hpp"

#include <ostream>

// Hashable key representing function name + function type
struct TypedSymbol {
  type::Type const *type;
  Symbol name;
  
  bool operator==(TypedSymbol const &rhs) const {
    return name == rhs.name && *type == *rhs.type;
  }
  
  bool operator!=(TypedSymbol const &rhs) const {
    return !(*this == rhs);
  }
};

template <>
struct std::hash<TypedSymbol> {
  size_t operator()(TypedSymbol const &id) const {
    std::hash<type::Type> hashType;
    std::hash<Symbol> hashSymbol;
    
    return (hashType(*id.type) << 4) ^ hashSymbol(id.name);
  }
};

// Output a mangled function name containing both the function name & type
std::ostream &operator<<(std::ostream &str, TypedSymbol const &sym);
