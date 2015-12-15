//
//  codegen-llvm.cpp
//  g-thunk
//
//  Created by Chris Devereux on 14/12/2015.
//
//

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/PassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"


/** Swift Interop **/

extern "C" {
#include "codegen-llvm.h"
}

// Complile-time checked casts between llvm type and matching opaque C types used from swift
#define api_cast_param(T, x) (llvm :: T) static_cast<T>(x)
#define api_cast_result(T, x) (T) static_cast<llvm :: T>(x)


/** Context Structs **/

struct Context {
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;
  llvm::LLVMContext &llCtx;
  
  Context()
  : builder(llvm::getGlobalContext())
  , module(llvm::make_unique<llvm::Module>("gthunk", llvm::getGlobalContext()))
  , llCtx(llvm::getGlobalContext())
  {
  }
};

struct ProcContext {
  Context *global;
  llvm::Function *llFn;
  
  ProcContext(Context *ctx, llvm::Function *fn)
  : global(ctx)
  , llFn(fn)
  {}
};


/** Global Context **/

Context *codegen_ctx_alloc(void) {
  return new Context();
}

JITFn codegen_jit(Context *ctx, Function *fn) {
  std::string error;
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  auto vm = llvm::EngineBuilder(std::move(ctx->module))
    .setEngineKind(llvm::EngineKind::JIT)
    .setVerifyModules(true)
    .setErrorStr(&error)
    .create()
  ;
  
  if (!vm) {
    fputs(error.c_str(), stderr);
    return nullptr;
  }
  
  vm->finalizeObject();
  
  auto fPtr = (JITFn)vm->getPointerToFunction(api_cast_param(Function *, fn));
  return fPtr;
}

void codegen_ctx_dealloc(Context *ctx) {
  assert(ctx);
  
  delete ctx;
}


/** Types **/

Type *codegen_type_f64(Context *ctx) {
  assert(ctx);
  
  return api_cast_result(Type *, llvm::Type::getDoubleTy(ctx->llCtx));
}


/** Procedure Context **/

ProcContext *proc_alloc(Context *ctx, char const *name, Type *resultType, Type *const * paramTypes, int arity) {
  assert(ctx); assert(name); assert(resultType); assert(paramTypes);
  
  llvm::ArrayRef<llvm::Type *> llParamTypes(api_cast_param(Type *const *, paramTypes), arity);
  
  auto type = llvm::FunctionType::get(api_cast_param(Type *, resultType), llParamTypes, false);
  auto fn = llvm::cast<llvm::Function>(ctx->module->getOrInsertFunction(name, type));
  fn->setLinkage(llvm::GlobalValue::LinkageTypes::ExternalLinkage);
  
  return new ProcContext(ctx, fn);
}

Function *proc_dealloc_and_complete(ProcContext *ctx) {
  assert(ctx);
  
  auto fn = ctx->llFn;
  llvm::verifyFunction(*fn);
  
  delete ctx;
  return api_cast_result(Function *, fn);
}

BasicBlock *proc_enter_block(ProcContext *ctx, char const *name) {
  assert(ctx); assert(name);
  
  auto block = llvm::BasicBlock::Create(ctx->global->llCtx, name, ctx->llFn);
  ctx->global->builder.SetInsertPoint(block);
  
  return api_cast_result(BasicBlock *, block);
}

void proc_set_return(ProcContext *ctx, Value *value) {
  assert(ctx); assert(value);
  
  ctx->global->builder.CreateRet(api_cast_param(Value *, value));
}


/** Value Context **/

Value *real_add(ProcContext *ctx, Value *lhs, Value *rhs) {
  assert(ctx); assert(lhs); assert(rhs);
  
  return api_cast_result(Value *, ctx->global->builder.CreateFAdd(api_cast_param(Value *, lhs), api_cast_param(Value *, rhs)));
}

Value *real(ProcContext *ctx, double value) {
  assert(ctx);
  
  return api_cast_result(Value *, llvm::ConstantFP::get(ctx->global->llCtx, llvm::APFloat(value)));
}

