#include "Type.hpp"
#include "Util.hpp"

namespace type {
  Type::~Type()
  {}
  
  Type::Visitor::~Visitor()
  {}
  
  Type const *AnyType::get() {
    static AnyType instance;
    return &instance;
  }
  
  Type const *F32() {
    static Atomic instance(Symbol::get("F32"));
    return &instance;
  }
  
  
  /** Visitors **/
  void AnyType::visit(Type::Visitor *v) const {
    v->acceptAny(this);
  }
  
  void Atomic::visit(Type::Visitor *v) const {
    v->acceptAtomic(this);
  }
  
  void Function::visit(Type::Visitor *v) const {
    v->acceptFunction(this);
  }
  
  void Vector::visit(Type::Visitor *v) const {
    v->acceptVector(this);
  }
  
  
  /** Equality **/
  bool Function::operator==(const type::Type &rhs) const {
    if (&rhs == this) return true;
    
    auto that = dynamic_cast<Function const *>(&rhs);
    if (!that) return false;
    
    return (*this->result == *that->result)
    && equalCollections(this->params, that->params, equalData<type::Type>)
    ;
  }
  
  bool AnyType::operator==(const type::Type &rhs) const {
    return &rhs == this;
  }
  
  bool Atomic::operator==(const type::Type &rhs) const {
    return &rhs == this;
  }
  
  bool Vector::operator==(const type::Type &rhs) const {
    auto that = dynamic_cast<Vector const *>(&rhs);
    if (!that) return false;
    
    return *this->innerType == *that->innerType;
  }
  
  
  /** Hash **/
  size_t AnyType::hashValue() const {
    return (size_t)this;
  }
  
  size_t Atomic::hashValue() const {
    return (size_t)this;
  }
  
  size_t Vector::hashValue() const {
    return innerType->hashValue() ^ 0xF0F0F0;
  }
  
  size_t Function::hashValue() const {
    size_t hash = this->result->hashValue();
    size_t shift = 3;
    
    for (auto x : this->params) {
      hash ^= x->hashValue() << shift;
      shift += 5;
    }
    
    return hash;
  }
  
  
  /** Conversions **/
  Type const *Type::scalarVersion() const {
    return this;
  }
  
  Type const *Vector::scalarVersion() const {
    return innerType;
  }
  
  Type const *Type::vectorVersion(Arena *arena) const {
    return arena->create<Vector>(this);
  }
  
  Type const *Vector::vectorVersion(Arena *arena) const {
    return this;
  }
  
  bool Type::isVector() const {
    return false;
  }
  
  bool Vector::isVector() const {
    return true;
  }
  
  Function const *Type::functionVersion(Arena *arena) const {
    return arena->create<Function>(this, Arena::vector<Type const *>(arena->allocator<Type const *>()));
  }
  
  Function const *Function::functionVersion(Arena *arena) const {
    return this;
  }
  
  
  /** Subtyping **/
  bool Atomic::subtypeOf(const type::Type *supertype) const {
    return this == supertype || supertype == AnyType::get();
  }
  
  bool AnyType::subtypeOf(const type::Type *supertype) const {
    return true;
  }
  
  bool Function::subtypeOf(const type::Type *supertype) const {
    if (this == supertype || supertype == AnyType::get()) return true;
    if (getArity() == 0 && result->subtypeOf(supertype)) return true;
    
    auto supertypeFn = dynamic_cast<Function const *>(supertype);
    if (!supertypeFn) return false;
    
    return result->subtypeOf(supertypeFn->result)
    && equalCollections(supertypeFn->params, params, std::mem_fn(&Type::subtypeOf));
  }
  
  bool Vector::subtypeOf(const type::Type *supertype) const {
    if (this == supertype || supertype == AnyType::get()) return true;
    
    auto vec = dynamic_cast<Vector const *>(supertype);
    if (vec) {
      return innerType->subtypeOf(vec->innerType);
      
    } else {
      return innerType->subtypeOf(supertype);
    }
  }
  
  
  
  /** Operations **/
  
  Type const *intersectionType(Type const *lhs, Type const *rhs) {
    if (lhs->subtypeOf(rhs)) {
      return lhs;
    }
    
    if (rhs->subtypeOf(lhs)) {
      return rhs;
    }
    
    return nullptr;
  }
}
