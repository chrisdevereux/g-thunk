#pragma once

#include <string>
#include <iostream>

class Symbol {
public:
  static Symbol get(char const *str);
  
  template <typename Traits, typename Allocator>
  static Symbol get(std::basic_string<char, Traits, Allocator> const &str) {
    return get(str.c_str());
  }
  
  bool operator==(const Symbol &rhs) const {
    return id == rhs.id;
  }
  
  bool operator!=(const Symbol &rhs) const {
    return !(*this == rhs);
  }
  
  bool operator<(const Symbol &rhs) const {
    return id < rhs.id;
  }
  
  operator std::string() const;
  
private:
  friend struct std::hash<Symbol>;
  uint32_t id;
  
  friend std::ostream &operator<<(std::ostream &str, Symbol const &sym);
};

template <>
struct std::hash<Symbol> {
  size_t operator()(const Symbol &sym) const {
    return std::hash<uint32_t>()(sym.id);
  }
};

std::ostream &operator<<(std::ostream &str, Symbol const &sym);
