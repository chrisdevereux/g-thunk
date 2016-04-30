#pragma once

#include "Data.hpp"

namespace vm {
  /**
   VM State
   
   The VMState type manages the interface between instruction execution and the VM's
   internal state.
   
   The VM is stack-based and maintains two separate stacks:
  
      * The SCALAR STACK holds typed fixed-size (8 byte) slots.
        Slots have 4 bytes available for type information and metadata, and 4 bytes for the
        data payload.
   
      * The VECTOR STACK has variable-sized, cache-aligned slots containing raw binary data.
        Slots are sized by the current frame size.
   
   This distinction is isolated from instruction execution. The Execution environment interacts
   only with the scalar stack directly -- From its point of view, the vector stack functions
   like a reference-counted heap.
   
   Certain scalar slot types act as references to vector data. These can be dereferenced to
   return a pointer to a vector buffer. Multiple scalar slots may reference the same vector
   slot. When a strong reference to a vector buffer is popped from the stack, its corresponding
   vector slot is popped.
   
   The VM should therefore preserve the following invariants:
   
      1) Vector slots are have exactly one strong reference.
      2) No weak references to a vector exist below the vector's strong reference.
      3) Scalar slots are always popped in LIFO order.
   */
  
  
  // Type flag for scalar stack slots
  enum SlotType : uint8_t {
    StrongVecRef,
    WeakVecRef,
    ScalarFP
  };
  
  
  // Scalar stack slot type
  struct ScalarStackSlot {
    union Payload {
      uint32_t vectorStackRef;
      float scalarFP;
    };
    static_assert(sizeof(Payload) <= 4 && alignof(Payload) <= 4, "Scalar payload should have size & alignment <= 4");
    
    SlotType type;
    Data::Value payload;
  };
  
  static_assert(sizeof(ScalarStackSlot) == 8, "Scalar stack slots should be 64bits");
  
  
  // Cache-aligned slot for vector data
  struct alignas(64) VectorStackSlot {
    // # samples per vector slot
    static size_t const SampleCount = 64 / sizeof(float);
    
    uint8_t data[64];
  };
  
  
  // VM state interface
  class VMState {
  public:
    VMState(ScalarStackSlot *scalarStack_, uint32_t scalarStackTop_,
            VectorStackSlot *vectorStack_, uint32_t vectorStackTop_,
            uint32_t sampleCount_)
    : vectorStack(vectorStack_)
    , stack(scalarStack_)
    , vectorStackTop(vectorStackTop_)
    , stackSize(scalarStackTop_) {
      frameSlots = (sampleCount_ % VectorStackSlot::SampleCount == 0)
      ? sampleCount_ / VectorStackSlot::SampleCount
      : sampleCount_ / VectorStackSlot::SampleCount + 1
      ;
    }
    
    // Allocate a new vector buffer, push a reference onto the stack,
    // and return the reference.
    ScalarStackSlot alloc();
    
    // Push a weak reference to the buffer referenced by `ref` onto the stack
    // and return the reference.
    ScalarStackSlot reference(ScalarStackSlot ref);
    
    // Obtain a pointer to the vector buffer referenced by `ref`
    Data::Value *dereference(ScalarStackSlot ref);
    
    // Get the slot at the n-th from top slot (1 is the top).
    // It is fine to get the last popped slot (0) if the caller popped it.
    ScalarStackSlot &get(uint32_t offset);
    
    // Return the current top slot
    uint32_t stackTop();
    
    // Push a new scalar onto the stack.
    void push(ScalarStackSlot data);
    
    // Pop the topmost scalar.
    void pop();
    
    // Pop n slots from the top (and any strongly referenced vectors)
    void pop(uint32_t count);
    
    // Return the length (in # of samples) of vectors in the current frame.
    uint64_t frameSamples() const {
      return frameSlots * VectorStackSlot::SampleCount;
    }
    
  private:
    // Pop the vector referenced by `data`
    void dealloc(ScalarStackSlot data);
    
    // Base of the vector stack.
    VectorStackSlot *vectorStack;
    
    // Base of the scalar stack.
    ScalarStackSlot *stack;
    
    // Number of vector slots in a vector in the current frame.
    // Equal to # samples / vector slot size
    uint32_t frameSlots;
    
    // Current scalar stack index.
    uint32_t stackSize;
    
    // Current vector stack index (in vector slots)
    uint32_t vectorStackTop;
  };
  
  
  ScalarStackSlot &VMState::get(uint32_t offset) {
    return stack[stackSize - offset];
  }
  
  void VMState::push(ScalarStackSlot data) {
    stack[stackSize] = data;
    ++stackSize;
  }
  
  void VMState::pop() {
    assert(stackSize != 0);
    
    --stackSize;
    
    if (stack[stackSize].type == StrongVecRef) {
      dealloc(stack[stackSize]);
    }
  }
  
  void VMState::pop(uint32_t count) {
    while (count > 0) {
      --count;
      pop();
    }
  }
  
  uint32_t VMState::stackTop() {
    return stackSize - 1;
  }
  
  
  ScalarStackSlot VMState::alloc() {
    ScalarStackSlot ref;
    ref.type = StrongVecRef;
    ref.payload.u32 = vectorStackTop;
    
    vectorStackTop += frameSlots;
    push(ref);
    
    return ref;
  }
  
  void VMState::dealloc(ScalarStackSlot ref) {
    assert(ref.type == StrongVecRef);
    assert(vectorStackTop - frameSlots == ref.payload.u32);
    
    vectorStackTop -= frameSlots;
  }
  
  ScalarStackSlot VMState::reference(ScalarStackSlot ref) {
    assert(ref.type == StrongVecRef || ref.type == WeakVecRef);
    
    return {WeakVecRef, ref.payload};
  }
  
  Data::Value *VMState::dereference(ScalarStackSlot ref) {
    assert(ref.type == StrongVecRef || ref.type == WeakVecRef);
    assert(vectorStackTop - frameSlots == ref.payload.u32);
    
    return (Data::Value *)(vectorStack + ref.payload.u32);
  }
}
