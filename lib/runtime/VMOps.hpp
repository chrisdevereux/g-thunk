#pragma once

#include <Accelerate/Accelerate.h>
#include "Data.hpp"

namespace vm {
  /** 
   Binary Operations.
   
   Each operation comes in 4 flavours, for each combination of vector & scalar operands.
  */
  
  struct Add {
    // Vector - Vector
    void operator()(float const *lhs, float const *rhs, float *output, size_t sampleCount) const {
      vDSP_vadd(lhs, 1, rhs, 1, output, 1, sampleCount);
    }
    
    // Vector - Scalar
    void operator()(float const *lhs, float const rhs, float *output, size_t sampleCount) const {
      vDSP_vsadd(lhs, 1, &rhs, output, 1, sampleCount);
    }
    
    // Scalar - Vector
    void operator()(float const lhs, float const *rhs, float *output, size_t sampleCount) const {
      vDSP_vsadd(rhs, 1, &lhs, output, 1, sampleCount);
    }
    
    // Scalar - Scalar
    void operator()(float lhs, float rhs, float *output) const {
      *output = lhs + rhs;
    }
  };
  
  struct Multiply {
    // Vector - Vector
    void operator()(float const *lhs, float const *rhs, float *output, size_t sampleCount) const {
      vDSP_vmul(lhs, 1, rhs, 1, output, 1, sampleCount);
    }
    
    // Vector - Scalar
    void operator()(float const *lhs, float const rhs, float *output, size_t sampleCount) const {
      vDSP_vsmul(lhs, 1, &rhs, output, 1, sampleCount);
    }
    
    // Scalar - Vector
    void operator()(float const lhs, float const *rhs, float *output, size_t sampleCount) const {
      vDSP_vsmul(rhs, 1, &lhs, output, 1, sampleCount);
    }
    
    // Scalar - Scalar
    void operator()(float lhs, float rhs, float *output) const {
      *output = lhs * rhs;
    }
  };
}
