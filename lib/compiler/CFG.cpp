#include "CFG.hpp"
#include "Util.hpp"

namespace cfg {
  /** Visitors **/
  
  void CallFunc::visit(Value::Visitor *visitor) const {
    visitor->acceptCall(this);
  }
  
  void PrimitiveOp::visit(Value::Visitor *visitor) const {
    visitor->acceptPrimitive(this);
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
  
  void PrimitiveOp::visit(Value::MutatingVisitor *visitor) {
    visitor->acceptPrimitive(this);
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
  
  bool Function::operator==(Function const &rhs) const {
    return *root == *rhs.root
    && paramCount == rhs.paramCount
    ;
  }
  
  bool Package::operator==(Package const &rhs) const {
    return exports == rhs.exports;
  }
  
  bool CallFunc::operator==(Value const &rhs) const {
    auto that = dynamic_cast<CallFunc const *>(&rhs);
    if (!that) return false;
    
    return *this->function == *that->function
    && equalCollections(this->params, that->params, equalData<Value>)
    ;
  }
  
  bool PrimitiveOp::operator==(Value const &rhs) const {
    auto that = dynamic_cast<PrimitiveOp const *>(&rhs);
    if (!that) return false;
    
    return this->operation == that->operation
    && equalCollections(this->operands, that->operands, equalData<Value>)
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
    
    return this->value == that->value && this->precision == that->precision;
  }
}
