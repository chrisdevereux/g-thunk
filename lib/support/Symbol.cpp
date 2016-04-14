#include "Symbol.hpp"

#include <cstring>
#include <cassert>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <experimental/string_view>

namespace {
  std::mutex lock;
  
  std::unordered_map<std::experimental::string_view, uint32_t> table;
  std::vector<char const *> symbolValues;
}

Symbol Symbol::get(const char *str) {
  std::experimental::string_view searchKey(str, strlen(str));
  std::lock_guard<std::mutex> guard(lock);
  
  auto iter = table.find(searchKey);
  Symbol sym;
  
  if (iter != table.end()) {
    sym.id = iter->second;
    
  } else {
    size_t len = strlen(str);
    auto storage = (char *)malloc(len + 1);
    strcpy(storage, str);
    
    assert(symbolValues.size() <= UINT32_MAX);
    sym.id = (uint32_t)symbolValues.size();
    
    std::experimental::string_view storedKey(storage, len);
    table[storedKey] = sym.id;
    symbolValues.push_back(storage);
  }
  
  return sym;
}

Symbol::operator std::string() const {
  std::lock_guard<std::mutex> guard(lock);
  return symbolValues[id];
}

std::ostream &operator<<(std::ostream &str, Symbol const &sym) {
  std::lock_guard<std::mutex> guard(lock);
  
  if (sym.id >= symbolValues.size()) {
    return str << "<invalid symbol>";
    
  } else {
    return str << symbolValues[sym.id];
  }
}
