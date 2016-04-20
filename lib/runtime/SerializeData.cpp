#include "SerializeData.hpp"

namespace vm {
  namespace unserialize {
    using namespace parse;
    
    void writeValue(std::ostream &str, Data::Type type, Data::Value value) {
      switch (type) {
        case Data::U32Value:
          str << value.u32;
          break;
          
        case Data::F32Value:
          str << value.f32;
          break;
          
        case Data::SymbolValue:
          str << value.sym;
          break;
      }
    }
    
    Grammar data(GenericAction<Data> out) {
      return [=](State const &state) -> Result {
        Data result;
        std::vector<float> value;
        
        return state
        >> match("{") >> optionalWhitespace
        >> delimited(real<float>(collect(&value)), whitespace)
        >> optionalWhitespace >> match("}")
        >> inject([&]{ result = Data(Data::F32Value, value.begin(), value.end()); })
        >> emit(&result, out)
        ;
      };
    }
  }
}

std::ostream &operator<<(std::ostream &str, vm::Data const &data) {
  str << "{";
  
  bool first = true;
  for (auto x : data.values) {
    if (!first) str << " ";
    vm::unserialize::writeValue(str, data.type, x);
    
    first = false;
  }
  
  str << "}";
  
  return str;
}
