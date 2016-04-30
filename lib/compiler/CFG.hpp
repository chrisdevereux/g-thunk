#pragma once

#include "TypedSymbol.hpp"
#include "Arena.hpp"
#include "Type.hpp"

namespace cfg {
  struct Value {
    struct Visitor;
    struct MutatingVisitor;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const = 0;
    
    virtual void visit(Visitor *visitor) const = 0;
    virtual void visit(MutatingVisitor *visitor) = 0;
    virtual bool operator==(Value const &rhs) const = 0;
    
    inline bool operator!=(Value const &rhs) {
      return !(*this == rhs);
    }
  };
  
  class Package {
  public:
    typedef Arena::unordered_map<TypedSymbol, Value *>::value_type Record;
    
    Package(Arena *arena)
    : functions(arena->allocator<Record>())
    {}
    
    Arena::unordered_map<TypedSymbol, Value *> functions;
    
    bool operator==(Package const &rhs) const;
    
    inline bool operator!=(Package const &rhs) {
      return !(*this == rhs);
    }
  };
  
  
  /** Value types */
  
  struct CallFunc : Value {
    Value *function;
    Arena::vector<Value *> params;
    
    explicit CallFunc(Arena *arena)
    : params(arena->allocator<Value *>())
    {}
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const;
  };
  
  struct BinaryOp : Value {
    enum Operation {
      ADD, MUL
    };
    
    Operation operation;
    Value *lhs;
    Value *rhs;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const;
  };
  
  struct FunctionRef : Value {
    Symbol name;
    type::Function const *type;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Function const *typeInFunction(type::Function const *fn) const;
  };
  
  struct IndirectValue : Value {
    Value *actualValue;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const;
  };
  
  struct ParamRef : Value {
    ParamRef()
    {}
    
    explicit ParamRef(size_t i)
    : index(i)
    {}
    
    size_t index;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const;
  };
  
  struct FPValue : Value {
    double value;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
    
    virtual type::Type const *typeInFunction(type::Function const *fn) const;
  };
  
  
  /** Visitor **/
  
  struct Value::Visitor {
    virtual void acceptCall(CallFunc const *v) = 0;
    virtual void acceptBinaryOp(BinaryOp const *v) = 0;
    virtual void acceptFunctionRef(FunctionRef const *v) = 0;
    virtual void acceptParamRef(ParamRef const *v) = 0;
    virtual void acceptFPValue(FPValue const *v) = 0;
  };
  
  struct Value::MutatingVisitor {
    virtual void acceptCall(CallFunc *v) = 0;
    virtual void acceptBinaryOp(BinaryOp *v) = 0;
    virtual void acceptFunctionRef(FunctionRef *v) = 0;
    virtual void acceptParamRef(ParamRef *v) = 0;
    virtual void acceptFPValue(FPValue *v) = 0;
  };
};
