#include "Intrinsics.hpp"

#include <sstream>

namespace {
  // Add an intrunsic function to a CFG package and return it's root node.
  //
  // Template parameter T specifies the type of the CFG package.
  //  - package: CFG package to insert intrinsics into.
  //  - arena: Memory arena to allocate CFG nodes in.
  //  - name: Symbol name for the function.
  //  - returnType: Function return type.
  //  - paramList: Types of function parameters.
  template <typename T>
  T *addIntrinsic(cfg::Package &package, Arena *arena, char const *name, type::Type const *returnType, std::initializer_list<type::Type const *> paramList) {
    Arena::vector<type::Type const *> params(arena->allocator<type::Type const *>());
    params = paramList;
    
    auto type = arena->create<type::Function>(returnType, params);
    TypedSymbol key = {type, Symbol::get(name)};
    ;
    
    auto root = arena->create<T>();
    package.functions[key] = root;
    
    return root;
  }
  
  // Helper for inserting binary operator.
  //
  // Ads a cfg::BinaryOp value performing a specific opcode, with the specified
  // lhs, rhs and return types.
  void addBinaryOpIntrinsic(cfg::Package &package, Arena *arena, char const *name, vm::Instruction::Opcode op, type::Type const *lhs, type::Type const *rhs) {
    auto result = type::intersectionType(lhs, rhs);
    auto intrinsic = addIntrinsic<cfg::BinaryOp>(package, arena, name, result, {lhs, rhs});
    
    intrinsic->operation = op;
    intrinsic->lhs = arena->create<cfg::ParamRef>(0);
    intrinsic->rhs = arena->create<cfg::ParamRef>(1);
  }
}

namespace compiler {
  cfg::Package intrinsics(Arena *arena) {
    cfg::Package package(arena);
    
    auto F32 = type::F32();
    auto vF32 = F32->vectorVersion(arena);
    
#define BINARY_INTRINSIC_VARIANTS(SYMBOL, OPCODE_PREFIX) \
    addBinaryOpIntrinsic(package, arena, SYMBOL, vm::Instruction::OPCODE_PREFIX##_VV, vF32, vF32); \
    addBinaryOpIntrinsic(package, arena, SYMBOL, vm::Instruction::OPCODE_PREFIX##_SV, F32, vF32); \
    addBinaryOpIntrinsic(package, arena, SYMBOL, vm::Instruction::OPCODE_PREFIX##_VS, vF32, F32); \
    addBinaryOpIntrinsic(package, arena, SYMBOL, vm::Instruction::OPCODE_PREFIX##_SS, F32, F32);
    
    BINARY_INTRINSIC_VARIANTS("+", ADD);
    BINARY_INTRINSIC_VARIANTS("*", MUL);
    
#undef BINARY_INTRINSIC_VARIANTS
  
    return package;
  }
}
