#pragma once

#include "Symbol.hpp"
#include "Arena.hpp"

namespace vm {
  struct Data {
    enum Type : uint8_t {
      U32Value, F32Value, SymbolValue
    };
    
    union Value {
      Value() {}
      Value(float val): f32(val) {}
      Value(Symbol val): sym(val) {}
      Value(uint32_t val): u32(val) {}
      
      inline bool operator==(Value rhs) const {
        return u32 == rhs.u32;
      }
      
      inline bool operator!=(Value rhs) const {
        return !(*this == rhs);
      }
      
      float f32;
      Symbol sym;
      uint32_t u32;
      struct {
        uint16_t first;
        uint16_t second;
      } u16pair;
    };
    
    static_assert(sizeof(Value) == 4, "value must be 32 bite wide");
    
    inline Data() {}
    
    inline Data(Type type_, Value value)
    : type(type_)
    , values(1, value)
    {}

    template <typename Iter>
    Data(Type type, Iter begin, Iter end);
    
    inline bool operator==(Data const &rhs) const {
      return type == rhs.type && values == rhs.values;
    }
    inline bool operator!=(Data const &rhs) const {
      return !(*this == rhs);
    }
    
    Type type;
    std::vector<Value> values;
  };
}

template <typename Iter>
vm::Data::Data(Data::Type type_, Iter begin, Iter end)
: type(type_)
, values(begin, end)
{}
