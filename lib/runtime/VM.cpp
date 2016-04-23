#include "VM.hpp"

#include <sstream>

namespace vm {
  using Value = Data::Value;
  
  union Scalar {
    Value value;
    
    struct {
      uint16_t slot;
      uint16_t refCount;
      
      uint32_t byteOffset() const {
        return static_cast<uint32_t>(slot) * 64;
      }
    } vecRef;
  };
  
  template <typename Self, typename T>
  struct Ptr32 {
    uint32_t byteOffset = 0;
    
    bool operator==(Self rhs) const {
      return byteOffset == rhs.byteOffset;
    }
    
    bool operator!=(Self rhs) const {
      return !(*this == rhs);
    }
    
    Self& operator++() {
      byteOffset += sizeof(T);
      return *reinterpret_cast<Self *>(this);
    }
    
    Self& operator--() {
      byteOffset -= sizeof(T);
      return *reinterpret_cast<Self *>(this);
    }
    
    Self operator+(uint32_t rhs) const {
      Self next;
      next.byteOffset = byteOffset + (rhs * sizeof(T));
      return next;
    }
    
    Self &operator+=(uint32_t rhs) {
      byteOffset = byteOffset + (rhs * sizeof(T));
      return *reinterpret_cast<Self *>(this);
    }
    
    Self operator-(uint32_t rhs) const {
      Self next;
      next.byteOffset = byteOffset - (rhs * sizeof(T));
      return next;
    }
    
    Self &operator-=(uint32_t rhs) {
      byteOffset = byteOffset - (rhs * sizeof(T));
      return *reinterpret_cast<Self *>(this);
    }
  };
  
  struct ScalarPtr : Ptr32<ScalarPtr, Scalar> {};
  struct VectorPtr : Ptr32<VectorPtr, Value> {
    uint16_t slot() const {
      assert(byteOffset % 64 == 0);
      return byteOffset / 64;
    }
  };

  struct Add {
    using OperandType = float;
    float operator()(float lhs, float rhs) {
      return lhs + rhs;
    }
  };
  
  struct Multiply {
    using OperandType = float;
    float operator()(float lhs, float rhs) {
      return lhs * rhs;
    }
  };

  struct State {
    // Vector stack pointer
    VectorPtr vsPtr;
    
    // Scalar stack pointer.
    ScalarPtr ssPtr;
  };
  
  struct VM::Impl {
    uint8_t *scalarStack;
    uint8_t *vectorStack;
    std::vector<Instruction> code;
    std::unordered_map<Symbol, uint32_t> symbols;
    
    Impl(Package const &package, size_t stackSize)
    : scalarStack((uint8_t *)malloc((stackSize / 8)))
    , vectorStack((uint8_t *)malloc(stackSize + 64))
    , code(package.code.begin(), package.code.end())
    , symbols(package.symbols.begin(), package.symbols.end())
    {}
    
    ~Impl() {
      free(scalarStack);
      free(vectorStack);
    }
    
    
    /** Function execution **/
    
    uint32_t resolve(Symbol sym) const {
      auto it = symbols.find(sym);
      if (it == symbols.end()) {
        std::stringstream ss;
        ss << "Undefined symbol: `" << sym << "`";
        
        throw std::runtime_error(ss.str());
      }
      
      return it->second;
    }
    
    State call(uint32_t instPtr, uint32_t frameSize, State state) {
      assert(frameSize % (64 / sizeof(float)) == 0 && "Frames should have 64 byte alignment");
      
      while (true) {
        auto inst = code[instPtr];
        
        switch (inst.operation) {
          case Instruction::PUSH: {
            // Push operand onto the stack
            scalar(state.ssPtr)->value = inst.operand;
            ++state.ssPtr;
            
            break;
          }
            
          case Instruction::PUSH_SYM: {
            // Lookup symbol and push value onto the stack
            auto target = resolve(inst.operand.sym);
            
            scalar(state.ssPtr)->value.u32 = target;
            ++state.ssPtr;
            
            // Fixup instruction to skip symbol lookup in future
            code[instPtr].operation = inst.PUSH;
            code[instPtr].operand.u32 = target;
            
            break;
          }
            
          case Instruction::FILL: {
            // Replace the scalar value at the top with a vector
            auto scalarTop = *scalar(state.ssPtr - 1);
            --state.ssPtr;
            
            std::fill_n(vector(state.vsPtr), frameSize, scalarTop.value);
            state = storeVectorTop(state, frameSize);
            
            break;
          }
            
          case Instruction::COPY: {
            // Copy the n-th from top scalar to top
            auto offset = inst.operand.u32;
            
            *scalar(state.ssPtr) = *scalar(state.ssPtr - offset);
            ++state.ssPtr;
            
            break;
          }
            
          case Instruction::REF_VEC: {
            // Copy the n-th from top vector reference to top and bump the refcount
            auto offset = inst.operand.u32;
            auto parentRef = scalar(state.ssPtr - offset)->vecRef;
            
            scalar(state.ssPtr)->vecRef = {
              .slot = parentRef.slot,
              .refCount = static_cast<uint16_t>(parentRef.refCount + 1)
            };
            
            ++state.ssPtr;
            
            break;
          }
            
          case Instruction::CALL: {
            // Pop the function ptr from the stack and jump to it.
            auto site = scalar(state.ssPtr - 1)->value.u32;
            --state.ssPtr;
            
            state = call(site, frameSize, state);
            
            break;
          }
            
#define BINARY_OP_HANDLERS(OPCODE_PREFIX, OPERATION) \
case Instruction::OPCODE_PREFIX##_VV: state = vecVecOp(frameSize, state, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_VS: state = vecScalarOp(frameSize, state, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_SV: state = scalarVecOp(frameSize, state, OPERATION()); break; \
case Instruction::OPCODE_PREFIX##_SS: state = scalarScalarOp(state, OPERATION()); break;
            
            // Call through to binary op implementation
            
            BINARY_OP_HANDLERS(ADD, Add)
            BINARY_OP_HANDLERS(MUL, Multiply)
            
#undef BINARY_OP_HANDLERS
            
          case Instruction::EXIT_FIXUP_S: {
            auto scalarOffset = inst.operand.u16pair.first;
            auto vectorOffset = inst.operand.u16pair.second;
            
            *scalar(state.ssPtr - scalarOffset - 1) = *scalar(state.ssPtr - scalarOffset);
            state.ssPtr -= scalarOffset;
            state.vsPtr -= vectorOffset;
            
            return state;
          }
          case Instruction::EXIT_FIXUP_V: {
            auto scalarOffset = inst.operand.u16pair.first;
            auto vectorOffset = inst.operand.u16pair.second;
            auto outPtr = state.vsPtr - frameSize * (vectorOffset + 1);
            
            scalar(state.ssPtr - scalarOffset - 1)->vecRef = {
              .refCount = 1,
              .slot = outPtr.slot()
            };
            
            memcpy(vector(outPtr), vector(state.ssPtr - 1), frameSize * sizeof(float));
            
            state.ssPtr -= scalarOffset;
            state.vsPtr -= frameSize * vectorOffset;
            
            return state;
          }
          case Instruction::EXIT: {
            return state;
          }
        }
        
        instPtr++;
      }
    }
    
    
    /** Operations **/
    
    template <typename Op>
    State vecVecOp(uint32_t frameSize, State &state, Op op) {
      // vector-vector operation
      // - expects the top two scalar slots to be vector references
      
      using T = typename Op::OperandType;
      
      auto vecTop = vector<T>(state.ssPtr - 1);
      auto vecBtm = vector<T>(state.ssPtr - 2);
      
      state = popVectorRef(state, frameSize);
      state = popVectorRef(state, frameSize);
      
      auto out = vector<T>(state.vsPtr);
      
      for (size_t i = 0; i < frameSize; ++i) {
        out[i] = op(vecTop[i], vecBtm[i]);
      }
      
      return storeVectorTop(state, frameSize);
    }
    
    template <typename Op>
    State vecScalarOp(uint32_t frameSize, State state, Op op) {
      // vector-scalar operation
      // - expects the top scalar to reference a vector
      // - expects the bottom scalar to be a value
      
      using T = typename Op::OperandType;
      
      auto vecTop = vector<T>(state.ssPtr - 1);
      auto scalBtm = *scalar<T>(state.ssPtr - 2);
      
      state = popVectorRef(state, frameSize);
      --state.ssPtr;
      
      auto out = vector<T>(state.vsPtr);
      
      for (size_t i = 0; i < frameSize; ++i) {
        out[i] = op(vecTop[i], scalBtm);
      }
      
      return storeVectorTop(state, frameSize);
    }
    
    template <typename Op>
    State scalarVecOp(uint32_t frameSize, State &state, Op op) {
      // scalar-vector operation
      // - expects the top scalar to be a value
      // - expects the bottom scalar to reference a vector
      
      using T = typename Op::OperandType;
      
      auto scalTop = *scalar<T>(state.ssPtr - 1);
      auto vecBtm = vector<T>(state.ssPtr - 2);
      
      --state.ssPtr;
      state = popVectorRef(state, frameSize);
      
      auto out = vector<T>(state.vsPtr);
      
      for (size_t i = 0; i < frameSize; ++i) {
        out[i] = op(scalTop, vecBtm[i]);
      }
      
      return storeVectorTop(state, frameSize);
    }
    
    template <typename Op>
    State scalarScalarOp(State &state, Op op) {
      // scalar-scalar operation
      
      using T = typename Op::OperandType;
      
      auto scalarTop = *scalar<T>(state.ssPtr - 1);
      auto scalarBtm = *scalar<T>(state.ssPtr - 2);
      
      --state.ssPtr;
      *scalar<T>(state.ssPtr - 1) = op(scalarTop, scalarBtm);
      
      return state;
    }
    
    
    /** Stack Manipulation **/
    
    State popVectorRef(State state, uint32_t frameSize) {
      --state.ssPtr;
      auto vecRef = scalar(state.ssPtr)->vecRef;
      
      if (vecRef.refCount == 1) {
        // Implies that the vector stack top is the referant of this vector
        
        state.vsPtr -= frameSize;
        assert(state.vsPtr.byteOffset == vecRef.byteOffset());
      }
      
      return state;
    }
    
    State storeVectorTop(State state, uint32_t frameSize) {
      scalar(state.ssPtr)->vecRef = {
        .slot = state.vsPtr.slot(),
        .refCount = 1
      };
      
      state.vsPtr += frameSize;
      ++state.ssPtr;
      
      return state;
    }
    
    template <typename T = Scalar>
    T *scalar(ScalarPtr ptr) const {
      std::enable_if<sizeof(T) == sizeof(Scalar)>();
      return reinterpret_cast<T *>(scalarStack + ptr.byteOffset);
    }
    
    template <typename T = Value>
    T *vector(VectorPtr ptr) const {
      std::enable_if<sizeof(T) == sizeof(Value)>();
      return reinterpret_cast<T *>(vectorStack + ptr.byteOffset);
    }
    
    template <typename T = Value>
    T *vector(ScalarPtr ptr) const {
      std::enable_if<sizeof(T) == sizeof(Value)>();
      
      VectorPtr vecPtr;
      vecPtr.byteOffset = scalar(ptr)->vecRef.byteOffset();
      
      return vector<T>(vecPtr);
    }
    
    template <typename T = Value>
    T *vector(Scalar sc) const {
      std::enable_if<sizeof(T) == sizeof(Value)>();
      
      VectorPtr vecPtr;
      vecPtr.byteOffset = sc.vecRef.byteOffset();
      
      return vector<T>(vecPtr);
    }
  };
  
  VM::VM(Package const &package, size_t stackSize)
  : impl(std::make_unique<Impl>(package, stackSize))
  {}
  
  VM::~VM()
  {}
  
  void VM::call(Symbol symbol, float const *input, float *output, uint32_t frameSize) {
    uint32_t alignedSize = frameSize - (frameSize % 16) + 16;
    
    if (alignedSize != frameSize) {
      // Zero the extra bytes
      std::fill_n(impl->vectorStack, alignedSize, (float)0);
    }
    
    std::copy_n((float const *)input, frameSize, (float *)impl->vectorStack);
    
    auto entryState = impl->storeVectorTop(State(), alignedSize);
    auto exitState = impl->call(impl->resolve(symbol), alignedSize, entryState);
    
    assert(exitState.ssPtr.byteOffset == sizeof(float));
    assert(exitState.vsPtr.byteOffset == alignedSize * sizeof(float));
    
    std::copy_n((float const *)impl->vectorStack, frameSize, output);
  }
  
  Data VM::call(Symbol symbol, Data const &input) {
    Data result;
    result.type = Data::F32Value;
    result.values.resize(input.values.size());
    
    call(symbol, (float const *)input.values.data(), (float *)result.values.data(), (uint32_t)input.values.size());
    return result;
  }
}
