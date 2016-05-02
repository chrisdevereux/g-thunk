#include "SerializeCFG.hpp"
#include "SerializeType.hpp"
#include "StringifyUtil.hpp"

#include <sstream>

namespace cfg {
  using namespace parse;
  
  // SSA tree value node
  template <typename Action>
  Grammar valueTree(Action action);
  
  // CFG stringifier
  struct CFGStringifier : Value::Visitor {
    Stringifier stringify;
    
    CFGStringifier(std::ostream &str)
    : stringify(str)
    {}
    
    // SSA value types
    virtual void acceptCall(CallFunc const *v);
    virtual void acceptBinaryOp(BinaryOp const *v);
    virtual void acceptFunctionRef(FunctionRef const *v);
    virtual void acceptParamRef(ParamRef const *v);
    virtual void acceptFPValue(FPValue const *v);
    
    void acceptPackageFunction(std::pair<TypedSymbol const, cfg::Value *> const *v);
    void acceptPackage(Package const *v);
  };
  
  
  /** Call Function **/
  
  template <typename Action>
  auto callParams(Action out) {
    return [=](State const &state) -> Result {
      return state
      >> whitespace
      >> delimited(valueTree(out), whitespace)
      ;
    };
  }
  
  // Parse
  template <typename Action>
  auto call(Action out) {
    return taggedSExp("call", [=](State const &state) -> Result {
      auto result = state.create<CallFunc>(state.arena);
      
      return state
      >> valueTree(receive(&result->function))
      >> optional(callParams(collect(&result->params)))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptCall(const cfg::CallFunc *v) {
    stringify.begin("call");
    
    stringify.compound(v->function, this);
    stringify.each(v->params, this);
    
    stringify.end();
  }
  
  
  /** Primitive Operation **/
  
  // Parse type
  template <typename Action>
  auto operation(Action out) {
    return [=](State const &state) -> Result {
#define BINARY_INTRINSIC_VARIANTS(OPCODE_PREFIX, SYM_PREFIX) \
state >> match(SYM_PREFIX "_vv") >> emitValue(vm::Instruction::OPCODE_PREFIX##_VV, out) \
?: state >> match(SYM_PREFIX "_sv") >> emitValue(vm::Instruction::OPCODE_PREFIX##_SV, out) \
?: state >> match(SYM_PREFIX "_vs") >> emitValue(vm::Instruction::OPCODE_PREFIX##_VS, out) \
?: state >> match(SYM_PREFIX "_ss") >> emitValue(vm::Instruction::OPCODE_PREFIX##_SS, out)
      
      
      return BINARY_INTRINSIC_VARIANTS(ADD, "add")
      ?: BINARY_INTRINSIC_VARIANTS(MUL, "mul")
      ;
      
#undef BINARY_INTRINSIC_VARIANTS
    };
  }
  
  // Stringify type
  char const *operationString(vm::Instruction::Opcode op) {
    switch (op) {
#define BINARY_INTRINSIC_VARIANTS(OPCODE_PREFIX, SYM_PREFIX) \
case vm::Instruction::OPCODE_PREFIX##_VV: return SYM_PREFIX "_vv"; \
case vm::Instruction::OPCODE_PREFIX##_VS: return SYM_PREFIX "_vs"; \
case vm::Instruction::OPCODE_PREFIX##_SV: return SYM_PREFIX "_sv"; \
case vm::Instruction::OPCODE_PREFIX##_SS: return SYM_PREFIX "_ss"; \

        BINARY_INTRINSIC_VARIANTS(ADD, "add");
        BINARY_INTRINSIC_VARIANTS(MUL, "mul");
        
#undef BINARY_INTRINSIC_VARIANTS
      default: {
        auto err = std::stringstream() << "Cannot serialize binary operator " << op;
        throw std::logic_error(err.str());
      }
    }
  }
  
  // Parse
  template <typename Action>
  auto primitive(Action out) {
    return sExp([=](State const &state) -> Result {
      auto result = state.create<BinaryOp>();
      
      return state
      >> operation(receive(&result->operation))
      >> whitespace
      >> valueTree(receive(&result->lhs))
      >> whitespace
      >> valueTree(receive(&result->rhs))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptBinaryOp(const cfg::BinaryOp *v) {
    stringify.begin();
    
    stringify.atom(operationString(v->operation));
    stringify.compound(v->lhs, this);
    stringify.compound(v->rhs, this);
    
    stringify.end();
  }
  
  
  /** FunctionRef Reference **/
  
  // Parse
  template <typename Action>
  auto functionRef(Action out) {
    return [=](State const &state) -> Result {
      auto result = state.create<FunctionRef>();
      
      return state
      >> identifierString(receive(&result->name))
      >> emit(&result, out)
      ;
    };
  }
  
  // Stringify
  void CFGStringifier::acceptFunctionRef(const cfg::FunctionRef *v) {
    stringify.atom(v->name);
  }
  
  
  /** Parameter Reference **/
  
  // Parse
  template <typename Action>
  auto paramRef(Action out) {
    return taggedSExp("param", [=](State const &state) -> Result {
      auto result = state.create<ParamRef>();
      
      return state
      >> integer(receive(&result->index))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptParamRef(const cfg::ParamRef *v) {
    stringify.begin("param");
    stringify.atom(v->index);
    stringify.end();
  }
  
  
  /** FPValue **/
  
  // Parse
  template <typename Action>
  Grammar scalar(Action out) {
    return taggedSExp("fp", [=](State const &state) -> Result {
      auto result = state.create<FPValue>();
      
      return state
      >> real(receive(&result->value))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptFPValue(const cfg::FPValue *v) {
    stringify.begin("fp");
    
    stringify.atom(v->value);
    
    stringify.end();
  }
  
  
  /** SSA Value Variant **/
  
  template <typename Action>
  Grammar valueTree(Action action) {
    return [=](State const &state) -> Result {
      return state >> call(action)
      ?: state >> primitive(action) >> log("primitive")
      ?: state >> call(action) >> log("call")
      ?: state >> paramRef(action) >> log("param")
      ?: state >> scalar(action) >> log("scalar")
      ?: state >> functionRef(action) >> log("ref")
      ;
    };
  }
  
  Grammar unserialize::value(GenericAction<Value *> out) {
    return valueTree(out);
  }
  
  
  /** Type declaration **/
  
  struct StringifyType : type::Type::Visitor {
    Stringifier stringify;
    
    virtual void acceptAny(type::AnyType const *t) {
      stringify.atom(t);
    }
    
    virtual void acceptAtomic(type::Atomic const *t) {
      stringify.atom(t);
    }
    
    virtual void acceptFunction(type::Function const *t) {
      stringify.begin("func");
      
      for (size_t i = 0 ; i < t->getArity(); ++i) {
        stringify.compound(t->getParamType(i), this);
      }
      
      stringify.end();
    }
    
    virtual void acceptVector(type::Vector const *t) {
      stringify.begin("vec");
      stringify.compound(t->getInnerType(), this);
      stringify.end();
    }
  };
  
  
  
  
  
  
  /** Package **/
  
  // Parse
  Grammar unserialize::package(GenericAction<Package> out) {
    return [=](State const &state) -> Result {
      Package result(state.arena);
      
      auto function = sExp([&](State const &state) {
        TypedSymbol key;
        cfg::Value *val;
        
        return state
        >> identifierString(receive(&key.name))
        >> whitespace
        >> type::unserialize(receive(&key.type))
        >> whitespace
        >> valueTree(receive(&val))
        >> inject([&]{ result.functions[key] = val; })
        ;
      });
      
      return state
      >> optionalWhitespace
      >> delimited(function, whitespace)
      >> optionalWhitespace
      >> emit(&result, out)
      ;
    };
  }
  
  // Stringify
  void CFGStringifier::acceptPackageFunction(std::pair<TypedSymbol const, cfg::Value *> const *v) {
    stringify.begin();
    stringify.atom(v->first.name);
    stringify.atom(v->first.type);
    stringify.compound(v->second, this);
    stringify.end();
  }
  
  void CFGStringifier::acceptPackage(const cfg::Package *v) {
    stringify.each(v->functions, this, &CFGStringifier::acceptPackageFunction);
  }
}

std::ostream &operator <<(std::ostream &str, const cfg::Package &package) {
  cfg::CFGStringifier serializer(str);
  serializer.acceptPackage(&package);
  
  return str;
}

std::ostream &operator <<(std::ostream &str, const cfg::Value &v) {
  cfg::CFGStringifier serializer(str);
  v.visit(&serializer);
  
  return str;
}
