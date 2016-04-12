#include "AST.hpp"
#include <functional>

namespace ast {
  Expression::~Expression()
  {}
  
  
  /** Casts **/
  
  Apply const *Expression::toApply() const {
    return nullptr;
  }
  
  Apply const *Apply::toApply() const {
    return this;
  }
  
  
  /** Comparisons **/
  
  template <typename T>
  bool equalPointers(T const *lhs, T const *rhs) {
    return *lhs == *rhs;
  }
  
  template <typename T, typename Compare>
  bool equalVecs(Arena::vector<T> const &lhs, Arena::vector<T> const &rhs, Compare const &compare) {
    return lhs.size() == rhs.size()
    && std::equal(lhs.begin(), lhs.end(), rhs.begin(), compare)
    ;
  }
  
  bool Module::operator==(const Module &rhs) const {
    return declarations == rhs.declarations;
  }
  
  bool Declaration::operator==(const Declaration &rhs) const {
    return name == rhs.name && *value == *rhs.value;
  }
  
  bool Scalar::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<Scalar const *>(&rhs);
    if (!that) return false;
    
    return this->value == that->value;
  }
  
  bool Identifier::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<Identifier const *>(&rhs);
    if (!that) return false;
    
    return this->value == that->value;
  }
  
  bool Apply::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<Apply const *>(&rhs);
    if (!that) return false;
    
    return *this->function == *that->function
    && equalVecs(this->params, that->params, equalPointers<ast::Expression>);
  }
  
  bool Function::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<Function const *>(&rhs);
    if (!that) return false;
    
    return this->params == that->params
    && *this->value == *that->value;
  }
  
  bool LexicalScope::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<LexicalScope const *>(&rhs);
    if (!that) return false;
    
    return this->bindings == that->bindings
    && *this->value == *that->value;
  }
  
  bool Conditional::operator==(const ast::Expression &rhs) const {
    auto that = dynamic_cast<Conditional const *>(&rhs);
    if (!that) return false;
    
    return *this->condition == *that->condition
    && *this->trueBranch == *that->trueBranch
    && *this->falseBranch == *that->falseBranch;
  }
  
  
  /** Visitor Implementations **/
  
  void Scalar::visit(Expression::Visitor *visitor) const {
    visitor->acceptScalar(this);
  }
  void Identifier::visit(Expression::Visitor *visitor) const {
    visitor->acceptIdentifier(this);
  }
  void Apply::visit(Expression::Visitor *visitor) const {
    visitor->acceptApply(this);
  }
  void Function::visit(Expression::Visitor *visitor) const {
    visitor->acceptFunction(this);
  }
  void LexicalScope::visit(Expression::Visitor *visitor) const {
    visitor->acceptLexicalScope(this);
  }
  void Conditional::visit(Expression::Visitor *visitor) const {
    visitor->acceptConditional(this);
  }
}
