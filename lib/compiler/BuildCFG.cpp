#include "BuildCFG.hpp"
#include "Type.hpp"
#include "Intrinsics.hpp"

#include <sstream>

namespace {
  /**
   BuildCFG
   
   Starting at a root function and a type signature, traverses all referenced
   functions, building a typed CFG for each.
   
   CFG functions are identified by typed symbol -- the union of function
   type signature and name.
   
   AST function definitions are treated as templates, reified into typed
   functions as part of the BuildCFG transformation by instantiating the most
   specific variant of the function that satisfied all type constraints.
   */

  
  
  /** Context Types **/
  
  // Top-level BuildCFG context object.
  //
  // Constructor parameters:
  //  - arena: Memory arena to allocate build CFG from.
  //  - module: AST source to resolve unbuilt functions from
  //  - package: Package to emit built functions to.
  //
  class GlobalContext {
  public:
    GlobalContext(Arena *arena_, ast::Module *module, cfg::Package *package_)
    : arena(arena_)
    , sources(arena->allocator<decltype(sources)::value_type>())
    , package(package_)
    {
      for (auto decl : module->declarations) {
        sources[decl.name] = decl.value;
      }
    }
    
    // Lookup the root CFG value for the function named `name` of type
    // `requestedType`. If it has either been built already or build is in progress,
    // return the cfg root. Otherwise, lookup the source AST and start building.
    cfg::Value *resolveIdentifier(Symbol name, type::Function const *requestedType);
    
    Arena *const arena;
    
  private:
    Arena::unordered_map<Symbol, ast::Expression const *> sources;
    cfg::Package *package;
  };
  
  
  // Scope-level BuildCFG context object.
  //
  // Constructor parameters:
  //  - global: Global BuildCFG context
  //  - function: Type signature of the enclosing function.
  //  - paramNames: Variable names for the enclosing function's parameters
  //
  class ScopeContext {
  public:
    ScopeContext(GlobalContext *global_, type::Function const *function_, Arena::vector<Symbol> const &paramNames)
    : arena(global_->arena)
    , function(function_)
    , global(global_)
    , bindings(arena->allocator<decltype(bindings)::value_type>())
    {
      size_t i = 0;
      for (auto p : paramNames) {
        auto val = arena->create<cfg::ParamRef>();
        val->index = i;
        
        bindings[p] = val;
        ++i;
      }
    }
    
    // Try to resolve `identifier` through parent lexical scopes. Otherwise, lookup
    //
    cfg::Value *resolveIdentifier(Symbol identifier, type::Type const *requestedType);
    
    // Try to reify `expr` as `requestedType` and return the CFG value on success.
    // Currently throws an exception on failure in lieu of proper error reporting.
    cfg::Value *build(ast::Expression const *expr, type::Type const *requestedType);
    
    Arena *const arena;
    type::Function const *const function;
    
  private:
    GlobalContext *global;
    Arena::unordered_map<Symbol, cfg::Value *> bindings;
  };
  
  
  // AST visitor. Transforms an AST expression into a CFG value.
  struct ExpressionToValue : ast::Expression::Visitor {
    ScopeContext *context;
    type::Type const *requestedType;
    cfg::Value *outputValue = nullptr;
    
    virtual void acceptScalar(ast::Scalar const *s) {
      auto scalar = create<cfg::FPValue>();
      scalar->value = s->value;
      
      outputValue = scalar;
    }
    
    virtual void acceptIdentifier(ast::Identifier const *s) {
      outputValue = context->resolveIdentifier(s->value, requestedType);
    }
    
    virtual void acceptOperatorSequence(ast::OperatorSequence const *s) {
      throw std::logic_error("Operator sequence should be removed before ast -> cfg transorm");
    }
    
    virtual void acceptFunction(ast::Function const *s) {
      throw std::runtime_error("Lambda expressions are not supported yet");
    }
    
    virtual void acceptApply(ast::Apply const *s) {
      using namespace std::placeholders;
      
      // Reify call parameters
      Arena::vector<cfg::Value *> params(context->arena->allocator<cfg::Value *>());
      std::transform(s->params.begin(),
                     s->params.end(),
                     std::back_inserter(params),
                     std::bind(&ScopeContext::build, context, _1, type::AnyType::get()));
      
      // Construct function type constraint
      Arena::vector<type::Type const *> paramTypes(context->arena->allocator<type::Type const *>());
      std::transform(params.begin(),
                     params.end(),
                     std::back_inserter(paramTypes),
                     std::bind(&cfg::Value::typeInFunction, _1, context->function));
      
      // Reify the called function
      auto fnType = create<type::Function>(requestedType, paramTypes);
      auto fnSite = context->build(s->function, fnType);
      
      // Build the output object.
      auto callFn = create<cfg::CallFunc>(context->arena);
      callFn->params = params;
      callFn->function = fnSite;
      
      outputValue = callFn;
    }
    
    virtual void acceptLexicalScope(ast::LexicalScope const *s) {
      throw std::runtime_error("Lexical scopes are not supported yet");
    }
    
    template <typename T, typename ...Params>
    T *create(Params ...params) {
      return context->arena->create<T>(params...);
    }
  };
  
  
  /** ScopeContext Implementation **/
  
  cfg::Value *ScopeContext::resolveIdentifier(Symbol identifier, type::Type const *requestedType) {
    // Try to resolve locally first.
    auto localHit = bindings.find(identifier);
    if (localHit != bindings.end()) {
      return localHit->second;
    }
    
    // If not found locally, try to resolve globally.
    //
    // Since global symbols are all functions, a non-function global is implemented
    // as an implicitly called 0-ary function.
    
    auto requestedFunctionType = dynamic_cast<type::Function const *>(requestedType);
    
    if (requestedFunctionType) {
      // If we're resolving a function, just lookup the function and return a reference
      // to it
      global->resolveIdentifier(identifier, requestedType->functionVersion(arena));
      
      auto val = arena->create<cfg::FunctionRef>();
      val->name = identifier;
      val->type = requestedFunctionType;
      
      return val;
      
    } else {
      // If we're resolving a non-function value, lookup a 0-ary function and emit
      // an CFG value to call it.
      requestedFunctionType = requestedType->functionVersion(arena);
      
      global->resolveIdentifier(identifier, requestedFunctionType);
      
      auto val = arena->create<cfg::FunctionRef>();
      val->name = identifier;
      val->type = requestedFunctionType;
      
      auto callFn = arena->create<cfg::CallFunc>(arena);
      callFn->function = val;
      
      return callFn;
    }
  }
  
  cfg::Value *ScopeContext::build(ast::Expression const *expr, type::Type const *requestedType) {
    // Visit the AST node
    ExpressionToValue visitor;
    visitor.context = this;
    visitor.requestedType = requestedType;
    
    expr->visit(&visitor);
    
    // Assert result.
    // Visitor should always return a correct CFG type.
    assert(visitor.outputValue);
    assert(visitor.outputValue->typeInFunction(function)->subtypeOf(requestedType));
    assert(visitor.outputValue->typeInFunction(function) != type::AnyType::get());
    
    return visitor.outputValue;
  }
  
  
  /** GlobalContext Implementation **/
  
  cfg::Value *GlobalContext::resolveIdentifier(Symbol identifier, type::Function const *requestedType) {
    TypedSymbol key = {requestedType, identifier};
    
    // First try: lookup cached cfg.
    auto cacheHit = package->functions.find(key);
    
    if (cacheHit != package->functions.end()) {
      return cacheHit->second;
    }
    
    // Second try: build cfg from source.
    auto sourceHit = sources.find(identifier);
    
    if (sourceHit != sources.end()) {
      auto expr = sourceHit->second;
      auto &fn = package->functions[key];
      
      auto fnExpr = dynamic_cast<ast::Function const *>(expr);
      if (fnExpr) {
        // AST node is a function definition. Bind function parameters.
        fn = ScopeContext(this, requestedType, fnExpr->params).build(fnExpr->value, requestedType->getResultType());
        
      } else {
        // AST node is not a function definition. No function parameters to bind.
        Arena::vector<Symbol> paramNames(arena->allocator<Symbol>());
        fn = ScopeContext(this, requestedType, paramNames).build(expr, requestedType->getResultType());
      }
      
      return fn;
    }
    
    auto err = std::stringstream() << "Use of undeclared identifier: " << identifier;
    throw std::runtime_error(err.str());
  }
}

namespace compiler {
  cfg::Package buildCFG(ast::Module *module, Arena *arena, Symbol rootName, type::Function const *rootType) {
    // Initialize the CFG with intrinsic functions.
    cfg::Package package(compiler::intrinsics(arena));
    GlobalContext context(arena, module, &package);
    
    // Build the CFG, starting at main.
    context.resolveIdentifier(rootName, rootType);
    
    return package;
  }
}
