#include "SerializeInstruction.hpp"
#include "StringifyUtil.hpp"

namespace vm {
  namespace unserialize {
    using namespace parse;
    
    template <typename ValueAction, typename TypeAction>
    auto typedValue(ValueAction valueOut, TypeAction typeOut) {
      return [=](State const &state) -> Result {
        Data::Value result;
        
        return state
        >> match("f32") >> spaces
        >> real(receive(&result.f32))
        >> emitValue(Data::F32Value, typeOut)
        >> emit(&result, valueOut)
        ;
      };
    }
    
    std::ostream &stringifyTypedValue(std::ostream &str, Instruction inst) {
      switch (inst.operandType) {
        case Data::F32Value:
          return str << "f32 " << inst.operand.f32;
          
        case Data::U32Value:
        case Data::SymbolValue:
          throw std::logic_error("Unsupported operand type");
      }
    }
    
    template <typename Action>
    auto label(Action out) {
      return [=](State const &state) -> Result {
        Symbol result;
        return state >> match(exactly('.')) >> identifierString(receive(&result)) >> emit(&result, out);
      };
    }
    
    template <typename Action>
    auto instruction(Action out) {
      return [=](State const &state) -> Result {
        Instruction result;
        
        auto opcode = [&](Instruction::Opcode op) {
          return inject([&]{ result.operation = op; });
        };
        
        auto intOperand = integer<uint32_t>(receive(&result.operand.u32));
        auto symbolOperand = identifierString(receive(&result.operand.sym));
        auto typedOperand = typedValue(receive(&result.operand), receive(&result.operandType));
        auto u16PairOperand = integer<uint16_t>(receive(&result.operand.u16pair.first))
        >> spaces >> integer<uint16_t>(receive(&result.operand.u16pair.second))
        ;
        
        return state
        >> match("push")
        >> require("operand for push op", spaces >> typedOperand)
        >> opcode(Instruction::PUSH) >> emit(&result, out)
        
        ?: state
        >> match("push_sym")
        >> require("symbol name as operand for push_sym op", spaces >> symbolOperand)
        >> opcode(Instruction::PUSH_SYM) >> emit(&result, out)
        
        ?: state
        >> match("copy")
        >> require("slot offset as operand for copy op", spaces >> intOperand)
        >> opcode(Instruction::COPY) >> emit(&result, out)
        
        ?: state
        >> match("fill")
        >> opcode(Instruction::FILL) >> emit(&result, out)
        
        ?: state
        >> match("ref_vec")
        >> require("slot offset as operand for ref_vec op", spaces >> intOperand)
        >> opcode(Instruction::REF_VEC) >> emit(&result, out)
        
#define BinaryOpType(OPCODE_PREFIX, STR_PREFIX) \
?: state \
>> match(STR_PREFIX "_vv") \
>> opcode(Instruction::OPCODE_PREFIX##_VV) >> emit(&result, out) \
?: state \
>> match(STR_PREFIX "_sv") \
>> opcode(Instruction::OPCODE_PREFIX##_SV) >> emit(&result, out) \
?: state \
>> match(STR_PREFIX "_vs") \
>> opcode(Instruction::OPCODE_PREFIX##_VS) >> emit(&result, out) \
?: state \
>> match(STR_PREFIX "_ss") \
>> opcode(Instruction::OPCODE_PREFIX##_SS) >> emit(&result, out) \

        BinaryOpType(ADD, "add")
        BinaryOpType(MUL, "mul")
        
#undef BinaryOpType
        
        ?: state
        >> match("call")
        >> opcode(Instruction::CALL) >> emit(&result, out)
        
        ?: state
        >> match("exit_fixup_v")
        >> spaces >> u16PairOperand
        >> opcode(Instruction::EXIT_FIXUP_V) >> emit(&result, out)
        
        ?: state
        >> match("exit_fixup_s")
        >> spaces >> u16PairOperand
        >> opcode(Instruction::EXIT_FIXUP_S) >> emit(&result, out)
        
        ?: state
        >> match("exit") >> opcode(Instruction::EXIT) >> emit(&result, out)
        ;
      };
    }
    
    template <typename Symbol, typename Instruction>
    auto packageLine(Symbol labelOut, Instruction instructionOut) {
      return [=](State const &state) -> Result {
        return state >> label(labelOut)
        ?: state >> instruction(instructionOut)
        ;
      };
    }
    
    Grammar package(GenericAction<Package> out) {
      return [=](State const &state) -> Result {
        Package result(state.arena);
        uint32_t offset = 0;
        
        auto receiveInstruction = [&](Instruction inst) {
          result.code.push_back(inst);
          offset++;
        };
        
        auto receiveLabel = [&](Symbol sym) {
          result.symbols[sym] = offset;
        };
        
        return state
        >> optionalWhitespace
        >> delimited(optionalWhitespace >> packageLine(receiveLabel, receiveInstruction), newline)
        >> optionalWhitespace
        >> emit(&result, out);
      };
    }
  }
}

std::ostream &operator <<(std::ostream &str, vm::Instruction const &inst) {
  using vm::Instruction;
  using namespace vm::unserialize;
  
  switch (inst.operation) {
    case Instruction::PUSH:
      str << "push ";
      stringifyTypedValue(str, inst);
      return str;
      
    case Instruction::PUSH_SYM:
      return str << "push_sym " << inst.operand.sym;
      
    case Instruction::COPY:
      return str << "copy " << inst.operand.u32;
      
    case Instruction::FILL:
      return str << "fill";
      
    case Instruction::REF_VEC:
      return str << "ref_vec " << inst.operand.u32;
      
#define BinaryOpType(OPCODE_PREFIX, STR_PREFIX) \
case Instruction::OPCODE_PREFIX##_VV: return str << STR_PREFIX << "_vv"; \
case Instruction::OPCODE_PREFIX##_SV: return str << STR_PREFIX << "_sv"; \
case Instruction::OPCODE_PREFIX##_VS: return str << STR_PREFIX << "_vs"; \
case Instruction::OPCODE_PREFIX##_SS: return str << STR_PREFIX << "_ss";
      
      BinaryOpType(ADD, "add")
      BinaryOpType(MUL, "mul")
      
#undef BinaryOpType
      
    case Instruction::CALL:
      str << "call";
      return str;
      
    case Instruction::EXIT_FIXUP_S:
      return str << "exit_fixup_s "
      << inst.operand.u16pair.first << " "
      << inst.operand.u16pair.second
      ;
      
    case Instruction::EXIT_FIXUP_V:
      return str << "exit_fixup_v "
      << inst.operand.u16pair.first << " "
      << inst.operand.u16pair.second
      ;
      
    case Instruction::EXIT:
      return str << "exit";
  }
}

std::ostream &operator <<(std::ostream &str, vm::Package const &package) {
  using namespace vm::unserialize;
  
  std::unordered_multimap<uint32_t, Symbol> symbols;
  for (auto s : package.symbols) {
    symbols.insert(std::make_pair(s.second, s.first));
  }
  
  uint32_t i = 0;
  for (auto inst : package.code) {
    auto range = symbols.equal_range(i);
    
    if (range.first != range.second) {
      str << "\n";
    }
    
    for (auto it = range.first; it != range.second; ++it) {
      str << "." << it->second << "\n";
    }
    
    str << inst << "\n";
    ++i;
  }
  
  return str;
}
