#include "GC-CFG.hpp"
#include "Type.hpp"

#include <sstream>

// CFG visitor performing mark phase of garbage collection
struct MarkCFGFunctions : cfg::Value::Visitor {
  MarkCFGFunctions(Arena *arena, cfg::Package *package_)
  : marked(arena->allocator<TypedSymbol>())
  , package(package_)
  {}
  
  Arena::unordered_set<TypedSymbol> marked;
  cfg::Package *package;
  
  virtual void acceptCall(cfg::CallFunc const *v) {
    v->function->visit(this);
    
    for (auto p : v->params) {
      p->visit(this);
    }
  }
  
  virtual void acceptBinaryOp(cfg::BinaryOp const *v) {
    v->lhs->visit(this);
    v->rhs->visit(this);
  }
  
  virtual void acceptFunctionRef(cfg::FunctionRef const *v) {
    TypedSymbol key = {v->type, v->name};
    
    if (marked.find(key) != marked.end()) {
      return;
    }
    
    marked.insert(key);
    auto root = package->functions.find(key);
    if (root != package->functions.end()) {
      root->second->visit(this);
    }
  }
  
  virtual void acceptParamRef(cfg::ParamRef const *v) {
    
  }
  
  virtual void acceptFPValue(cfg::FPValue const *v) {
    
  }
};

namespace compiler {
  void gcCFG(Arena *arena, cfg::Package *package, TypedSymbol root)  {
    // Lookup the start function
    auto start = package->functions.find(root);
    if (start == package->functions.end()) {
      auto error = std::stringstream() << "Undefined function: " << root;
      throw std::logic_error(error.str());
    }
    
    // Mark used CFG nodes
    MarkCFGFunctions markVisitor(arena, package);
    markVisitor.marked.insert(root);
    
    start->second->visit(&markVisitor);
    
    // Sweap unused CFG nodes
    auto it = package->functions.begin();
    while (it != package->functions.end()) {
      if (markVisitor.marked.find(it->first) == markVisitor.marked.end()) {
        it = package->functions.erase(it);
        
      } else {
        ++it;
      }
    }
  }
}
