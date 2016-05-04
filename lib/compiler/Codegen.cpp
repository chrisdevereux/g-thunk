#include "Codegen.hpp"

#include <sstream>

using vm::Instruction;
using vm::Data;

namespace {
  // Flags passed to `emit()` to customize stack cleanup on function exit.
  enum ReturnFlags {
    // The returned value is a vector, so stack cleanup should use vector ops.
    VectorReturn = 1 << 0,
    
    // Some operations can perform their own stack cleanup by writing the result
    // to a specified stack slot and popping higher values. Some cannot do this
    // and require code emitted to perform explicit cleanup.
    ExplicitPop = 1 << 1
  };
  
  // bool -> VectorReturn flag conversion.
  int vecFlag(bool isVector) {
    return isVector ? VectorReturn : 0;
  }
  
  // Function-level codegen context.
  struct CodegenFunction {
    CodegenFunction(Arena *arena)
    : unusedParams(arena->allocator<uint32_t>())
    {}
    
    // Code output.
    Arena::vector<vm::Instruction> *code;
    
    // Type of the generated function.
    type::Function const *type;
    
    // # values that would be on the stack above the function's parameters
    // at runtime when the current instruction is executed.
    //
    // Value is invalid after code is emitted for a function's root cfg node.
    uint32_t stackSize = 0;
    
    // Set of unused function parameters. Used to determine stack cleanup
    // on exit.
    Arena::unordered_set<uint32_t> unusedParams;
  };
  
  class CodegenValue : public cfg::Value::Visitor {
  public:
    CodegenValue(CodegenFunction *context_, bool returnNode_)
    : context(context_)
    , returnNode(returnNode_)
    {}
    
  private:
    
    // Function-level context object.
    CodegenFunction *context;
    
    // True iff this is the root of the function's CFG.
    bool returnNode;
    
    
    /** CFG value visitors **/
    
    virtual void acceptCall(cfg::CallFunc const *v) {
      for (auto it = v->params.rbegin(); it != v->params.rend(); ++it) {
        emit(*it);
      }
      
      emit(v->function);
      
      emit(Instruction(Instruction::CALL, popCount()),
           vecFlag(v->hasVectorReturnInFunction(context->type)));
      
      popOperands(v->params.size() + 1);
    }
    
    
    virtual void acceptBinaryOp(cfg::BinaryOp const *v) {
      emit(v->rhs);
      emit(v->lhs);
      
      emit(Instruction(v->operation, popCount()),
           vecFlag(v->typeInFunction(context->type)->isVector()));
      
      popOperands(2);
    }
    
    virtual void acceptFunctionRef(cfg::FunctionRef const *v) {
      TypedSymbol sym = {v->type, v->name};
      auto mangledSym = Symbol::get((std::stringstream() << sym).str());
      
      emit(Instruction(Instruction::PUSH_SYM, mangledSym, Data::SymbolValue),
           ExplicitPop);
      
      pushValue();
    }
    
    virtual void acceptParamRef(cfg::ParamRef const *v) {
      auto paramType = v->typeInFunction(context->type);
      
      if (paramType->isVector()) {
        emit(Instruction(Instruction::REF_VEC, paramOffset(v->index)),
             VectorReturn | ExplicitPop);
        
      } else {
        emit(Instruction(Instruction::COPY, paramOffset(v->index)),
             ExplicitPop);
      }
      
      markUsed(v->index);
      pushValue();
    }
    
    virtual void acceptFPValue(cfg::FPValue const *v) {
      emit(Instruction(Instruction::PUSH, v->value, Data::F32Value),
           ExplicitPop);
      
      pushValue();
    }
    
    
    /** Helpers **/
    
    // Number of stack values that need to be popped before returning from
    // the function.
    uint32_t popCount() {
      return returnNode ? context->unusedParams.size() : 0;
    }
    
    
    // Emit code for `inst`, with function cleanup code required by `flags`
    // if this node is the function CFG root.
    void emit(vm::Instruction inst, int flags = 0) {
      if (returnNode) {
        // Wrap instruction in cleanup code if needed
        
        if (flags & ExplicitPop && popCount() != 0) {
          context->code->push_back(inst);
          context->code->push_back(Instruction::RET);
          
          auto opcode = (flags & VectorReturn) ? Instruction::DROP_V : Instruction::DROP_S;
          context->code->push_back(Instruction(opcode, context->unusedParams.size()));
          
        } else {
          context->code->push_back(Instruction::RET);
          context->code->push_back(inst);
        }
        
        context->unusedParams.clear();
        context->code->push_back(Instruction::EXIT);
        
      } else {
        // Otherwise, just emit the instruction
        
        context->code->push_back(inst);
      }
    }
    
    // Recurse into `value` and emit code at the current insertion point.
    void emit(cfg::Value const *value) {
      CodegenValue visitor(context, false);
      value->visit(&visitor);
    }
    
    // Increment runtime stack size counter.
    void pushValue() {
      ++context->stackSize;
    }
    
    // Decrement runtime stack size counter as an operation with n opcodes would.
    //
    // This assumes that the operation overwrites the lowest operand on the stack,
    // which isn't true after a function returns (if overwriting an unused parameter,
    // for example), so the stackSize counter is only valid before `emit` is called
    // on the root node.
    void popOperands(size_t count) {
      context->stackSize -= (count - 1);
    }
    
    // Stack offset of function parameter i
    uint32_t paramOffset(uint32_t paramIndex) {
      return context->stackSize + paramIndex + 1;
    }
    
    // Mark that a function parameter has been consumed by an operation.
    void markUsed(uint32_t paramIndex) {
      assert(returnNode || context->unusedParams.find(paramIndex) != context->unusedParams.end());
      context->unusedParams.erase(paramIndex);
    }
  };
}

namespace compiler {
  vm::Package codegen(cfg::Package const *sources, Arena *arena) {
    vm::Package package(arena);
    
    for (auto fn : sources->functions) {
      auto mangledSym = Symbol::get((std::stringstream() << fn.first).str());
      package.symbols[mangledSym] = package.code.size();
      
      CodegenFunction context(arena);
      context.code = &package.code;
      context.type = dynamic_cast<type::Function const *>(fn.first.type);
      
      for (size_t i = 0; i < context.type->getArity(); ++i) {
        context.unusedParams.insert(i);
      }
      
      CodegenValue root(&context, true);
      fn.second->visit(&root);
      
      assert(context.unusedParams.empty());
    }
    
    return package;
  }
}
