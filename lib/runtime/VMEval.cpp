#include "VMEval.hpp"
#include "VMOps.hpp"
#include "VMState.hpp"
#include "Instruction.hpp"
#include "SerializeInstruction.hpp"

namespace vm {
  template <typename Op>
  void vectorVectorOp(VMState *vm, uint32_t pop, Op op);
  
  template <typename Op>
  void vectorScalarOp(VMState *vm, uint32_t pop, Op op);
  
  template <typename Op>
  void scalarVectorOp(VMState *vm, uint32_t pop, Op op);
  
  template <typename Op>
  void scalarScalarOp(VMState *vm, uint32_t pop, Op op);
  
  
  // Lookup a symbol from package and return the instruction pointer
  uint32_t lookup(Package *package, Symbol sym) {
    auto hit = package->symbols.find(sym);
    
    if (hit == package->symbols.end()) {
      auto err = std::stringstream() << "Undefined symbol: `" << sym << "`";
      throw std::runtime_error(err.str());
      
    } else {
      return hit->second;
    }
  }
  
  
  // Main VM evaluation loop
  //
  // vm:        VM state object.
  // package:   Package containing code and symbol definitions.
  // InstPtr:   Pointer to first instruction.
  // popCount:  Overwrite n-many values from stack when returning.
  
  void eval(VMState *vm, Package *package, uint32_t instPtr, uint32_t popCount) {
    uint32_t resultOffset = 0;
    
    while (true) {
      auto inst = package->code[instPtr];
      
      switch (inst.operation) {
        case Instruction::PUSH:
          vm->push({ScalarFP, inst.operand});
          break;
          
        case Instruction::PUSH_SYM: {
          auto sym = inst.operand.sym;
          auto fPtr = lookup(package, sym);
          
          vm->push({ScalarFP, fPtr});
          
          // Fixup the instruction so future calls skip the symbol lookup
          package->code[instPtr] = Instruction(Instruction::PUSH, fPtr, Data::U32Value);
          
          break;
        }
        case Instruction::COPY:
          vm->push(vm->get(inst.operand.u32));
          break;
          
        case Instruction::REF_VEC:
          vm->push(vm->reference(vm->get(inst.operand.u32)));
          break;
          
        case Instruction::DROP_S: {
          // Consume the top slot + specified offset + return offset
          auto offset = inst.operand.u32 + resultOffset + 1;
          auto src = vm->get(1);
          
          vm->pop(offset);
          vm->push(src);
          
          break;
        }
        case Instruction::DROP_V: {
          // Consume the top slot + specified offset + return offset
          auto offset = inst.operand.u32 + resultOffset + 1;
          auto src = vm->get(1);
          
          auto newTop = 1 + vm->stackTop() - offset;
          
          if (newTop < src.payload.u32) {
            // Dropping a vector to below the location of its strong ref requires a copy.
            //
            // This should only happen in very rare cases, such as when a parameter
            // to a function invoked via polymorphic dispatch is returned immediately without
            // being modified.
            
            auto srcVec = vm->dereference(src);
            vm->pop(offset);
            
            auto destVec = vm->dereference(vm->alloc());
            std::copy_n(srcVec, vm->frameSamples(), destVec);
            
          } else if (newTop > src.payload.u32) {
            // Dropping a vector to a slot above the location of its strong ref just pushes a
            // an additional ref.
            
            vm->pop(offset);
            vm->push(vm->reference(src));
          }
          
          break;
        }
        case Instruction::FILL: {
          auto val = vm->get(1);
          vm->pop();
          
          auto ref = vm->alloc();
          std::fill_n(vm->dereference(ref), vm->frameSamples(), val.payload);
          
          break;
        }
        case Instruction::CALL: {
          auto fnPtr = vm->get(1).payload.u32;
          auto retSlot = inst.operand.u32 + resultOffset;
          
          vm->pop();
          eval(vm, package, fnPtr, retSlot);
          
          break;
        }
          
          // Handler for each variant of each binary operation:
#define BINARY_OP_VARIANTS(OPCODE_PREFIX, OPERATION) \
case Instruction::OPCODE_PREFIX##_VV: vectorVectorOp(vm, inst.operand.u32 + resultOffset, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_VS: vectorScalarOp(vm, inst.operand.u32 + resultOffset, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_SV: scalarVectorOp(vm, inst.operand.u32 + resultOffset, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_SS: scalarScalarOp(vm, inst.operand.u32 + resultOffset, OPERATION()); break;
          
          BINARY_OP_VARIANTS(ADD, Add);
          BINARY_OP_VARIANTS(MUL, Multiply);
          
#undef BINARY_OP_VARIANTS
          
        case Instruction::RET:
          resultOffset = popCount;
          break;
          
        case Instruction::EXIT:
          return;
      }
      
      ++instPtr;
    }
  }
  
  
  // Test function.
  //
  // Push a vector parameter onto the stack, execute a function and return the value.
  //
  //   package:     Package containing code.
  //   symbol:      Name of function to execute.
  //   param:       Parameter for the function.
  //   stackSize:   Stack sizes to use for evaluation (default 16k)
  
  Data eval(Package *package, Symbol symbol, Data const &param, size_t stackSize) {
    std::vector<ScalarStackSlot> scalarStack;
    scalarStack.resize(stackSize);
    
    std::vector<VectorStackSlot> vectorStack;
    vectorStack.resize(stackSize);
    
    VMState state(scalarStack.data(), 0, vectorStack.data(), 0, param.sampleCount());
    auto ref = state.alloc();
    
    std::copy_n(param.values.begin(), param.sampleCount(), state.dereference(ref));
    
    eval(&state, package, lookup(package, symbol), 0);
    
    Data result(param.type, param.sampleCount());
    std::copy_n(state.dereference(ref), param.sampleCount(), result.values.begin());
   
    return result;
  }
  
  
  /** Primitive Operation Helpers **/
  
  // Vector-Vector operation. Overwrite top 2 operands with the result of the
  // binary operation's vector-vector variant.
  //
  //   vm:        VM state object.
  //   pop:       Overwrite an additional n-many values from stack when returning.
  //   op:        Callable object defining the operation.
  
  template <typename Op>
  void vectorVectorOp(VMState *vm, uint32_t pop, Op op) {
    auto lhs = vm->get(1);
    auto rhs = vm->get(2);
    
    vm->pop(2 + pop);
    
    auto slot = vm->alloc();
    vm->push(slot);
    
    op((float const *)vm->dereference(lhs),
       (float const *)vm->dereference(rhs),
       (float *)vm->dereference(slot),
       vm->frameSamples());
  }
  
  
  // Vector-Scalar operation. Overwrite top 2 operands with the result of the
  // binary operation's vector-scalar variant.
  //
  //   vm:        VM state object.
  //   pop:       Overwrite an additional n-many values from stack when returning.
  //   op:        Callable object defining the operation.
  
  template <typename Op>
  void vectorScalarOp(VMState *vm, uint32_t pop, Op op) {
    auto lhs = vm->get(1);
    auto rhs = vm->get(2);
    
    vm->pop(2 + pop);
    
    auto slot = vm->alloc();
    vm->push(slot);
    
    op((float const *)vm->dereference(lhs),
       rhs.payload.f32,
       (float *)vm->dereference(slot),
       vm->frameSamples());
  }
  
  
  // Scalar-Vector operation. Overwrite top 2 operands with the result of the
  // binary operation's scalar-vector variant.
  //
  //   vm:        VM state object.
  //   pop:       Overwrite an additional n-many values from stack when returning.
  //   op:        Callable object defining the operation.
  
  template <typename Op>
  void scalarVectorOp(VMState *vm, uint32_t pop, Op op) {
    auto lhs = vm->get(1);
    auto rhs = vm->get(2);
    
    vm->pop(2 + pop);
    
    auto slot = vm->alloc();
    vm->push(slot);
    
    op(lhs.payload.f32,
       (float const *)vm->dereference(rhs),
       (float *)vm->dereference(slot),
       vm->frameSamples());
  }
  
  
  // Scalar-Scalar operation. Overwrite top 2 operands with the result of the
  // binary operation's scalar-scalar variant.
  //
  //   vm:        VM state object.
  //   pop:       Overwrite an additional n-many values from stack when returning.
  //   op:        Callable object defining the operation.
  
  template <typename Op>
  void scalarScalarOp(VMState *vm, uint32_t pop, Op op) {
    auto lhs = vm->get(1);
    auto rhs = vm->get(2);
    
    vm->pop(2 + pop);
    
    float result;
    op(lhs.payload.f32, rhs.payload.f32, &result);
    
    vm->push({ScalarFP, result});
  }
}
