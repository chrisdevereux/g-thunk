#pragma once

#include "Instruction.hpp"
#include "Data.hpp"
#include "Arena.hpp"

#include <memory>

namespace vm {
  class VM {
  public:
    struct Impl;
    
    explicit VM(Package const &package, size_t stackSize = 16 * 1024 * 1024);
    ~VM();
    
    void call(Symbol symbol, float const *input, float *output, uint32_t frameSize);
    Data call(Symbol symbol, Data const &input);
    
  private:
    std::unique_ptr<Impl> impl;
  };
}
