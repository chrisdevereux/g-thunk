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
        
        return state
        >> match("push_sym")
        >> require("symbol name as operand for push_sym op", spaces >> symbolOperand)
        >> opcode(Instruction::PUSH_SYM) >> emit(&result, out)
        
        ?: state
        >> match("push")
        >> require("operand for push op", spaces >> typedOperand)
        >> opcode(Instruction::PUSH) >> emit(&result, out)
        
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
        
        ?: state
        >> match("drop_s")
        >> require("slot offset as operand for drop_s op", spaces >> intOperand)
        >> opcode(Instruction::DROP_S) >> emit(&result, out)
        
        ?: state
        >> match("drop_v")
        >> require("slot offset as operand for drop_v op", spaces >> intOperand)
        >> opcode(Instruction::DROP_V) >> emit(&result, out)
        
#define BinaryOpType(OPCODE_PREFIX, STR_PREFIX) \
?: state \
>> match(STR_PREFIX "_vv") \
>> require("return offset as operand for " STR_PREFIX "_vv op", spaces >> intOperand) \
>> opcode(Instruction::OPCODE_PREFIX##_VV) \
>> emit(&result, out) \
\
?: state \
>> match(STR_PREFIX "_sv") \
>> require("return offset as operand for " STR_PREFIX "_sv op", spaces >> intOperand) \
>> opcode(Instruction::OPCODE_PREFIX##_SV) \
>> emit(&result, out) \
\
?: state \
>> match(STR_PREFIX "_vs") \
>> require("return offset as operand for " STR_PREFIX "_vs op", spaces >> intOperand) \
>> opcode(Instruction::OPCODE_PREFIX##_VS) \
>> emit(&result, out) \
\
?: state \
>> match(STR_PREFIX "_ss") \
>> require("return offset as operand for " STR_PREFIX "_ss op", spaces >> intOperand) \
>> opcode(Instruction::OPCODE_PREFIX##_SS) \
>> emit(&result, out) \

        BinaryOpType(ADD, "add")
        BinaryOpType(MUL, "mul")
        
#undef BinaryOpType
        
        ?: state
        >> match("call")
        >> require("return slot as operand for call op", spaces >> intOperand)
        >> opcode(Instruction::CALL) >> emit(&result, out)
        
        ?: state
        >> match("ret")
        >> opcode(Instruction::RET) >> emit(&result, out)
        
        ?: state
        >> match("exit")
        >> opcode(Instruction::EXIT) >> emit(&result, out)
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
      
    case Instruction::DROP_S:
      return str << "drop_s " << inst.operand.u32;
      
    case Instruction::DROP_V:
      return str << "drop_v " << inst.operand.u32;
      
#define BinaryOpType(OPCODE_PREFIX, STR_PREFIX) \
case Instruction::OPCODE_PREFIX##_VV: return str << STR_PREFIX << "_vv" << " " << inst.operand.u32; \
case Instruction::OPCODE_PREFIX##_SV: return str << STR_PREFIX << "_sv" << " " << inst.operand.u32; \
case Instruction::OPCODE_PREFIX##_VS: return str << STR_PREFIX << "_vs" << " " << inst.operand.u32; \
case Instruction::OPCODE_PREFIX##_SS: return str << STR_PREFIX << "_ss" << " " << inst.operand.u32;
      
      BinaryOpType(ADD, "add")
      BinaryOpType(MUL, "mul")
      
#undef BinaryOpType
      
    case Instruction::CALL:
      str << "call" << " " << inst.operand.u32;
      return str;
      
    case Instruction::RET:
      return str << "ret";
      
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
