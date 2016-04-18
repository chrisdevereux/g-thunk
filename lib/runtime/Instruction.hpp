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
      
      // Replace the value at the top of the stack with a vector containing all values.
      FILL,
      
      // Arithmetic ops.
      // Performs the aritmetic operation on the top two stack values.
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
      // Expects the operand value to be a symbol referencing a function.
      // Call to the function passing parameters from the stack in LIFO order.
      CALL,
      
      // Exit function.
      //
      // If a function uses all of its arguments, the last operation will write its output
      // to the function's output slot without any extra work.
      //
      // Fixup variants allow the return value to overwrite unused function parameters.
      // The operand is a pair of slot offsets for the overwritten scalar and vector slots
      EXIT_FIXUP_S,
      EXIT_FIXUP_V,
      EXIT
    };
    
    Opcode operation;
    Data::Type operandType;
    Data::Value operand;
    
    inline bool operator==(const Instruction &rhs) const {
      if (operation != rhs.operation) return false;
      
      switch (operation) {
        case PUSH:
          return operandType == rhs.operandType && operand.u32 == rhs.operand.u32;
          
        case COPY:
        case REF_VEC:
        case PUSH_SYM:
        case EXIT_FIXUP_S:
        case EXIT_FIXUP_V:
          return operand.u32 == rhs.operand.u32;
          
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
        case EXIT:
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
