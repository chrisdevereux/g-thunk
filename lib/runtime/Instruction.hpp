#pragma once

#include "Symbol.hpp"
#include "Arena.hpp"
#include "Data.hpp"

#include <cstddef>
#include <cstdint>

namespace vm {
  struct Instruction {
    // Opcode type
    enum Opcode : uint8_t {
      // Push value
      //
      // Push the operand value onto the top of the stack.
      PUSH,
      
      // Lookup the value of sym and push it onto the stack
      PUSH_SYM,
      
      // Copy the n-th from top stack value onto the stack.
      COPY,
      
      // Push an additional reference to the n-th from top vector onto the stack
      REF_VEC,
      
      // Drop stack slots in the range (top - n - 1)..(top - 1) so that the top
      // value drops n values. Requires that the top slot is a scalar.
      DROP_S,
      
      // Drop stack slots in the range (top - n - 1)..(top - 1) so that the top
      // value drops n values. Requires that the top slot is a vector.
      DROP_V,
      
      // Replace the value at the top of the stack with a vector containing all values.
      FILL,
      
      // Arithmetic ops.
      //
      // Performs the aritmetic operation on the top two stack values
      // and replace them with the result.
      //
      // Payload is a u32 stating how many additional stack slots
      // to pop when returning the value.
      //
      // Each operation comes in four flavours:
      //   VV - Vector-Vector
      //   SV - Scalar-Vector
      //   VS - Vector-Scalar
      //   SS - Scalar-Scalar
      
      // Addition
      ADD_VV,
      ADD_SV,
      ADD_VS,
      ADD_SS,
      
      // Multiplication
      MUL_VV,
      MUL_SV,
      MUL_VS,
      MUL_SS,
      
      // Call function
      //
      // Call the function referenced at stack top, passing parameters from
      // the stack with the first argument at the top.
      //
      // The callee should pop all operands from the stack before returning.
      //
      // Payload is a u32 stating how many additional stack slots
      // to pop when returning the value.
      CALL,
      
      
      // Signal that the next instruction is a function's return value, so the
      // VM should pop an additional n params from the stack (where n is the payload of
      // the CALL operation that called the function)
      //
      // No-op if executed in the main function.
      RET,
      
      // Return from the current function, or finish execution if in the main function.
      EXIT
    };
    
    Instruction() {}
    Instruction(Opcode operation_, Data::Value operand_, Data::Type operandType_ = Data::U32Value)
    : operation(operation_)
    , operandType(operandType_)
    , operand(operand_)
    {}
    
    explicit Instruction(Opcode operation_)
    : operation(operation_)
    {}
    
    Opcode operation;
    Data::Type operandType = Data::U32Value;
    Data::Value operand;
    
    inline bool operator==(const Instruction &rhs) const {
      if (operation != rhs.operation) return false;
      
      switch (operation) {
        case PUSH:
          if (operandType != rhs.operandType) return false;
          
          switch (operandType) {
            case Data::F32Value: return operand.f32 == rhs.operand.f32;
            case Data::U32Value: return operand.u32 == rhs.operand.u32;
            case Data::SymbolValue: return operand.sym == rhs.operand.sym;
          }
          
        case COPY:
        case REF_VEC:
        case DROP_S:
        case DROP_V:
        case PUSH_SYM:
        case CALL:
        case FILL:
        case ADD_VV:
        case ADD_VS:
        case ADD_SV:
        case ADD_SS:
        case MUL_VV:
        case MUL_VS:
        case MUL_SV:
        case MUL_SS:
          return operand.u32 == rhs.operand.u32;
          
        case EXIT:
        case RET:
          return true;
      }
    }
    
    inline bool operator!=(const Instruction &rhs) const {
      return !(*this == rhs);
    }
  };
  
  static_assert(sizeof(Instruction) == 8, "Expected instruction size to be 64 bits");
  
  struct Package {
    explicit Package(Arena *arena)
    : code(arena->allocator<Instruction>())
    , symbols(arena->allocator<std::pair<Symbol, size_t>>())
    {}
    
    bool operator==(Package const &rhs) const {
      return code == rhs.code && symbols == rhs.symbols;
    }
    
    inline bool operator!=(Package const &rhs) const {
      return !(*this == rhs);
    }
    
    Arena::vector<Instruction> code;
    Arena::unordered_map<Symbol, uint32_t> symbols;
  };
}
