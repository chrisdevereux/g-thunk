#include "CFG.hpp"
#include "Util.hpp"

namespace cfg {
  /** Visitors **/
  
  void CallFunc::visit(Value::Visitor *visitor) const {
    visitor->acceptCall(this);
  }
  
  void BinaryOp::visit(Value::Visitor *visitor) const {
    visitor->acceptBinaryOp(this);
  }
  
  void ParamRef::visit(Value::Visitor *visitor) const {
    visitor->acceptParamRef(this);
  }
  
  void FunctionRef::visit(Value::Visitor *visitor) const {
    visitor->acceptFunctionRef(this);
  }
  
  void FPValue::visit(Value::Visitor *visitor) const {
    visitor->acceptFPValue(this);
  }
  
  void CallFunc::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptCall(this);
  }
  
  void BinaryOp::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptBinaryOp(this);
  }
  
  void ParamRef::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptParamRef(this);
  }
  
  void FunctionRef::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptFunctionRef(this);
  }
  
  void FPValue::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptFPValue(this);
  }
  
  
  /** Comparisons **/
  bool Package::operator==(Package const &rhs) const {
    return equalCollections(functions, rhs.functions, [&](Record const &record, Record const &) {
      auto it = rhs.functions.find(record.first);
      return it != rhs.functions.end() && *it->second == *record.second;
    });
  }
  
  bool CallFunc::operator==(Value const &rhs) const {
    auto that = dynamic_cast<CallFunc const *>(&rhs);
    if (!that) return false;
    
    return *this->function == *that->function
    && equalCollections(this->params, that->params, equalData<Value>)
    ;
  }
  
  bool BinaryOp::operator==(Value const &rhs) const {
    auto that = dynamic_cast<BinaryOp const *>(&rhs);
    if (!that) return false;
    
    return this->operation == that->operation
    && *this->lhs == *that->lhs
    && *this->rhs == *that->rhs
    ;
  }
  
  bool ParamRef::operator==(Value const &rhs) const {
    auto that = dynamic_cast<ParamRef const *>(&rhs);
    if (!that) return false;
    
    return this->index == that->index;
  }
  
  bool FunctionRef::operator==(Value const &rhs) const {
    auto that = dynamic_cast<FunctionRef const *>(&rhs);
    if (!that) return false;
    
    return this->name == that->name;
  }
  
  bool FPValue::operator==(Value const &rhs) const {
    auto that = dynamic_cast<FPValue const *>(&rhs);
    if (!that) return false;
    
    return this->value == that->value;
  }
  
  
  /** Types **/
  
  type::Type const *CallFunc::typeInFunction(type::Function const *fn) const {
    auto fnType = dynamic_cast<type::Function const *>(function->typeInFunction(fn));
    return fnType ? fnType->getResultType() : nullptr;
  }
  
  type::Type const *BinaryOp::typeInFunction(type::Function const *fn) const {
    return type::intersectionType(lhs->typeInFunction(fn), rhs->typeInFunction(fn));
  }
  
  type::Function const *FunctionRef::typeInFunction(type::Function const *fn) const {
    return type;
  }
  
  type::Type const *ParamRef::typeInFunction(type::Function const *fn) const {
    return fn->getParamType(index);
  }
  
  type::Type const *FPValue::typeInFunction(type::Function const *fn) const {
    return type::F32();
  }
}
