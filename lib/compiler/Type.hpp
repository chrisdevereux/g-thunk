#pragma once

#include "Arena.hpp"
#include "Data.hpp"

namespace type {
  /**
    Type
   
    Represent the possible concrete types available to the VM.
   
    These types are assigned to the CFG on creation and used by the codegen stage
    to emit the appropriate VM instructions.
   
    Types exist in a hierarchy, rooted at `Any`, with subtypes.
    Polymorphism between coveraint types holds only at compile time.
   */
  
  class Function;
  
  class Type {
  public:
    struct Visitor;
    virtual ~Type();
    
    virtual bool operator==(Type const &rhs) const = 0;
    virtual size_t hashValue() const = 0;
    
    virtual void visit(Visitor *) const = 0;
    
    inline bool operator!=(Type const &rhs) const {
      return !(*this == rhs);
    }
    
    // Return true iff the type is a vector type.
    virtual bool isVector() const;
    
    // Return the scalar form of the type if a vector,
    // otherwise, return this.
    virtual Type const *scalarVersion() const;
    
    // Return the vector form of the type if a scalar,
    // otherwise, return this.
    virtual Type const *vectorVersion(Arena *) const;
    
    // Return the function form (for type x, () -> x) of the type if not a function,
    // otherwise return this.
    virtual Function const *functionVersion(Arena *) const;
    
    // True iff values of this type are polymorphic with `supertype` at compile-time.
    virtual bool subtypeOf(Type const *supertype) const = 0;
  };
  
  
  // Any type
  //
  // The only abstract type in the type system.
  // CFG values should never be assigned this type.
  //
  // Subtype of:
  //  - <none>
  //
  class AnyType : public Type {
  public:
    static Type const *get();
    
    virtual void visit(Visitor *) const;
    virtual bool operator==(Type const &rhs) const;
    virtual size_t hashValue() const;
    
    virtual bool subtypeOf(Type const *supertype) const;
    
  private:
    AnyType() {};
  };

  
  // An atomic type, identified by pointer identity.
  //
  // Well-known atomic types are obtained using the factory
  // functions in this module. (F32(), etc)
  //
  // Subtype of:
  //  - AnyType
  //
  class Atomic : public Type {
  public:
    Atomic(Symbol tag_)
    : tag(tag_)
    {}
    
    virtual void visit(Visitor *) const;
    virtual bool operator==(Type const &rhs) const;
    virtual size_t hashValue() const;
    
    virtual bool subtypeOf(Type const *supertype) const;
    Symbol getTag() const {
      return tag;
    }
    
  private:
    Symbol tag;
  };
  
  
  // Function type.
  //
  // Subtype of:
  //  - AnyType
  //  - If no arguments, the return value type.
  //  - Any function type with covariant return value type and contravariant arguments.
  //
  class Function : public Type {
  public:
    Function(Type const *result_, Arena::vector<Type const *> params_)
    : params(params_)
    , result(result_)
    {}
    
    size_t const getArity() const { return params.size(); };
    Type const *getResultType() const { return result; }
    Type const *getParamType(size_t index) const { return params[index]; }
    
    virtual void visit(Visitor *) const;
    virtual bool operator==(Type const &rhs) const;
    virtual Function const *functionVersion(Arena *) const;
    virtual size_t hashValue() const;
    
    virtual bool subtypeOf(Type const *supertype) const;
    
  private:
    Arena::vector<Type const *> params;
    Type const *result;
  };
  
  
  // Vector type.
  //
  // Subtype of:
  //  - AnyType
  //  - Scalar version of same type
  //
  class Vector : public Type {
  public:
    Vector(Type const *innerType_)
    : innerType(innerType_->scalarVersion())
    {}
    
    Type const *getInnerType() const { return innerType; }
    
    virtual void visit(Visitor *) const;
    virtual bool operator==(Type const &rhs) const;
    virtual size_t hashValue() const;
    
    virtual bool isVector() const;
    virtual Type const *scalarVersion() const;
    virtual Type const *vectorVersion(Arena *) const;
    
    virtual bool subtypeOf(Type const *supertype) const;
    
  private:
    Type const *innerType;
  };
  
  
  /** Atomic types **/
  
  // 32-bit floating point
  Type const *F32();
  
  
  struct Type::Visitor {
    virtual ~Visitor();
    
    virtual void acceptAny(AnyType const *) = 0;
    virtual void acceptAtomic(Atomic const *) = 0;
    virtual void acceptFunction(Function const *) = 0;
    virtual void acceptVector(Vector const *) = 0;
  };
  
  
  Type const *intersectionType(Type const *lhs, Type const *rhs);
}

template <>
struct std::hash<type::Type> {
  size_t operator()(const type::Type &t) const {
    return t.hashValue();
  }
};


