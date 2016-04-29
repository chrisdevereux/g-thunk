#pragma once

#include "Arena.hpp"
#include "Symbol.hpp"

#include <vector>
#include <string>

namespace ast {
  struct Expression;
  struct Apply;

  struct Declaration {
    Symbol name;
    Expression const *value;
    
    Declaration()
    : value(nullptr)
    {}
    
    bool operator ==(const Declaration &rhs) const;
    
    inline bool operator!=(const Declaration &rhs) const {
      return !(*this == rhs);
    }
  };
  
  struct Module {
    Arena::vector<Declaration> declarations;
    
    explicit Module(Arena *arena)
    : declarations(arena->allocator<Declaration>())
    {}
    
    bool operator ==(const Module &rhs) const;
    
    inline bool operator!=(const Module &rhs) const {
      return !(*this == rhs);
    }
  };
  
  struct Expression {
  public:
    class Visitor;
    virtual ~Expression();
    
    virtual void visit(Visitor *visitor) const = 0;
    virtual bool operator ==(const Expression &rhs) const = 0;
    
    virtual const Apply *toApply() const;
    
    inline bool operator!=(const Expression &rhs) const {
      return !(*this == rhs);
    }
  };
  
  struct Scalar : public Expression {
    double value;
    
    virtual void visit(Visitor *visitor) const;
    virtual bool operator ==(const Expression &rhs) const;
  };
  
  struct Identifier : public Expression {
    Symbol value;
    
    virtual void visit(Visitor *visitor) const;
    virtual bool operator ==(const Expression &rhs) const;
  };
  
  struct Apply : public Expression {
    Expression const *function;
    Arena::vector<Expression const *> params;
    
    explicit Apply(Arena *arena)
    : params(arena->allocator<Expression const *>())
    {}
    
    virtual void visit(Visitor *visitor) const;
    virtual bool operator ==(const Expression &rhs) const;
    
    virtual Apply const *toApply() const;
  };
  
  struct Function : public Expression {
    Arena::vector<Symbol> params;
    Expression const *value;
    
    explicit Function(Arena *arena)
    : params(arena->allocator<Symbol>())
    {}
    
    virtual void visit(Visitor *visitor) const;
    virtual bool operator ==(const Expression &rhs) const;
  };
  
  struct LexicalScope : public Expression {
    Expression const *value;
    Arena::vector<Declaration> bindings;
    
    explicit LexicalScope(Arena *arena)
    : bindings(arena->allocator<Declaration>())
    {}
    
    virtual void visit(Visitor *visitor) const;
    virtual bool operator ==(const Expression &rhs) const;
  };
  
  class Expression::Visitor {
  public:
    virtual void acceptScalar(Scalar const *s) = 0;
    virtual void acceptIdentifier(Identifier const *s) = 0;
    virtual void acceptFunction(Function const *s) = 0;
    virtual void acceptApply(Apply const *s) = 0;
    virtual void acceptLexicalScope(LexicalScope const *s) = 0;
  };
}
