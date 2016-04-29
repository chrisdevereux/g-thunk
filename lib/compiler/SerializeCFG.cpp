#include "SerializeCFG.hpp"
#include "StringifyUtil.hpp"

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
    virtual void acceptPrimitive(PrimitiveOp const *v);
    virtual void acceptFunctionRef(FunctionRef const *v);
    virtual void acceptParamRef(ParamRef const *v);
    virtual void acceptFPValue(FPValue const *v);
    
    void acceptFunction(Function const *v);
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
  Grammar call(Action out) {
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
  Grammar operation(Action out) {
    return [=](State const &state) -> Result {
      return state >> match("add") >> emitValue(PrimitiveOp::Add, out)
      ?: state >> match("mul") >> emitValue(PrimitiveOp::Multiply, out)
      ;
    };
  }
  
  // Stringify type
  char const *operationString(PrimitiveOp::Operation op) {
    switch (op) {
      case PrimitiveOp::Add: return "add";
      case PrimitiveOp::Multiply: return "mul";
    }
  }
  
  // Parse
  template <typename Action>
  Grammar primitive(Action out) {
    return sExp([=](State const &state) -> Result {
      auto result = state.create<PrimitiveOp>(state.arena);
      
      return state
      >> operation(receive(&result->operation))
      >> whitespace
      >> delimited(valueTree(collect(&result->operands)), whitespace)
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptPrimitive(const cfg::PrimitiveOp *v) {
    stringify.begin();
    
    stringify.atom(operationString(v->operation));
    stringify.each(v->operands, this);
    
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
  Grammar paramRef(Action out) {
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
      >> integer(receive(&result->precision))
      >> whitespace
      >> real(receive(&result->value))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptFPValue(const cfg::FPValue *v) {
    stringify.begin("fp");
    
    stringify.atom(v->precision);
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
  
  
  
  /** Function **/
  
  // Parse parameter type
  template <typename Action>
  Grammar typeExpr(Action out) {
    Type placeholderType;
    
    return [=](State const &state) -> Result {
      return state
      >> identifierString(noop<Arena::string>())
      >> emit(&placeholderType, out)
      ;
    };
  }
  
  // Parse
  template <typename Action>
  Grammar function(Action out) {
    return sExp([=](State const &state) -> Result {
      Function result;
      
      return state
      >> identifierString(receiveSymbol(&result.name))
      >> whitespace
      >> integer(receive(&result.paramCount))
      >> whitespace
      >> valueTree(receive(&result.root))
      >> emit(&result, out)
      ;
    });
  }
  
  // Stringify
  void CFGStringifier::acceptFunction(const cfg::Function *v) {
    stringify.begin();
    
    stringify.atom(v->name);
    stringify.atom(v->paramCount);
    stringify.compound(v->root, this);
    
    stringify.end();
  }
  
  
  /** Package **/
  
  // Parse
  Grammar unserialize::package(GenericAction<Package> out) {
    return [=](State const &state) -> Result {
      Package result(state.arena);
      
      return state
      >> optionalWhitespace
      >> delimited(function(collect(&result.exports)), whitespace)
      >> optionalWhitespace
      >> emit(&result, out)
      ;
    };
  }
  
  // Stringify
  void CFGStringifier::acceptPackage(const cfg::Package *v) {
    stringify.each(v->exports, this, &CFGStringifier::acceptFunction);
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
