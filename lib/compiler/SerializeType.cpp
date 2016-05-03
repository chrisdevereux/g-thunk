#include "SerializeType.hpp"

using namespace parse;
using namespace type;

namespace {
  template <typename Action>
  Grammar typeTree(Action action);
  
  struct WriteMangledTypeName : Type::Visitor {
    WriteMangledTypeName(std::ostream &str_)
    : str(str_)
    {}
    
    std::ostream &str;
    
    virtual void acceptAny(AnyType const *);
    virtual void acceptAtomic(Atomic const *t);
    virtual void acceptFunction(Function const *t);
    virtual void acceptVector(Vector const *t);
  };
  
  
  
  void WriteMangledTypeName::acceptVector(Vector const *t) {
    str << "v" << t->getInnerType();
  }
  
  template <typename Action>
  auto vectorType(Action out) {
    return [=](State const &state) -> Result {
      type::Type const *innerType;
      type::Vector const *result;
      
      return state
      >> match("v") >> typeTree(receive(&innerType))
      >> inject([&]{ result = state.create<type::Vector>(innerType); })
      >> emit(&result, out)
      ;
    };
  }
  
  
  template <typename Action>
  auto f32Type(Action out) {
    return match("F32") >> emitValue(type::F32(), out);
  }
  
  void WriteMangledTypeName::acceptAtomic(Atomic const *t) {
    str << t->getTag();
  }
  
  
  template <typename Action>
  Grammar anyType(Action out) {
    return match("Any") >> emitValue(type::AnyType::get(), out);
  }
  
  void WriteMangledTypeName::acceptAny(AnyType const *) {
    str << "Any";
  }
  
  
  template <typename Action>
  auto functionType(Action out) {
    return [=](State const &state) -> Result {
      Arena::vector<type::Type const *> types(state.allocator<type::Type const *>());
      type::Function const *result;
      
      return state
      >> match("[")
      >> delimited(typeTree(collect(&types)), match(":"))
      >> match("]")
      >> inject([&]{
        Arena::vector<type::Type const *> params(state.allocator<type::Type const *>());
        params.assign(types.begin(), types.end() - 1);
        
        result = state.create<type::Function>(types.back(), params);
      })
      >> emit(&result, out)
      ;
    };
  }
  
  void WriteMangledTypeName::acceptFunction(Function const *t) {
    str << "[";
    
    for (size_t i = 0; i < t->getArity(); ++i) {
      str << t->getParamType(i) << ":";
    }
    
    str << t->getResultType();
    
    str << "]";
  }
  
  
  template <typename Action>
  Grammar typeTree(Action action) {
    return [=](State const &state) -> Result {
      return state >> vectorType(action) >> log("vec")
      ?: state >> anyType(action) >> log("any")
      ?: state >> f32Type(action) >> log("f32")
      ?: state >> functionType(action) >> log("fn")
      ;
    };
  }
}

namespace type {
  namespace unserialize {
    parse::Grammar type(parse::GenericAction<type::Type const *> out) {
      return typeTree(out);
    }
    
    parse::Grammar function(parse::GenericAction<type::Function const *> out) {
      return functionType(out);
    }
  }
}

std::ostream &operator<<(std::ostream &str, type::Type const *type) {
  WriteMangledTypeName visitor(str);
  type->visit(&visitor);
  
  return str;
}
