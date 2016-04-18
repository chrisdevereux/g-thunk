#pragma once

#include "Symbol.hpp"
#include "Arena.hpp"
#include "Type.hpp"

namespace cfg {
  struct Value {
    struct Visitor;
    struct MutatingVisitor;
    
    virtual void visit(Visitor *visitor) const = 0;
    virtual void visit(MutatingVisitor *visitor) = 0;
    virtual bool operator==(Value const &rhs) const = 0;
    
    inline bool operator!=(Value const &rhs) {
      return !(*this == rhs);
    }
  };
  
  struct Function {
    Symbol name;
    Value *root;
    size_t paramCount;
    
    bool operator==(Function const &rhs) const;
    
    inline bool operator!=(Function const &rhs) {
      return !(*this == rhs);
    }
  };
  
  struct Package {
    Arena::vector<Function> exports;
    
    Package(Arena *arena)
    : exports(arena->allocator<Function>())
    {}
    
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
  };
  
  struct PrimitiveOp : Value {
    enum Operation {
      Add, Multiply
    };
    
    Operation operation;
    Arena::vector<Value *> operands;
    
    explicit PrimitiveOp(Arena *arena)
    : operands(arena->allocator<Value *>())
    {}
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
  };
  
  struct FunctionRef : Value {
    Symbol name;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
  };
  
  struct ParamRef : Value {
    size_t index;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
  };
  
  struct FPValue : Value {
    double value;
    size_t precision;
    
    virtual void visit(Visitor *visitor) const;
    virtual void visit(MutatingVisitor *visitor);
    virtual bool operator==(Value const &rhs) const;
  };
  
  
  /** Visitor **/
  
  struct Value::Visitor {
    virtual void acceptCall(CallFunc const *v) = 0;
    virtual void acceptPrimitive(PrimitiveOp const *v) = 0;
    virtual void acceptFunctionRef(FunctionRef const *v) = 0;
    virtual void acceptParamRef(ParamRef const *v) = 0;
    virtual void acceptFPValue(FPValue const *v) = 0;
  };
  
  struct Value::MutatingVisitor {
    virtual void acceptCall(CallFunc *v) = 0;
    virtual void acceptPrimitive(PrimitiveOp *v) = 0;
    virtual void acceptFunctionRef(FunctionRef *v) = 0;
    virtual void acceptParamRef(ParamRef *v) = 0;
    virtual void acceptFPValue(FPValue *v) = 0;
  };
};
