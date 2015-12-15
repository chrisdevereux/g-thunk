//
//  codegen-llvm.swift
//  g-thunk
//
//  Created by Chris Devereux on 15/12/2015.
//
//


func compileProgram(program: Program) -> JITFn {
  let ctx = codegen_ctx_alloc()
  let main = program.procedures[program.entry]
  
  let llMain = codegen(ctx, value: main)
  let fptr = codegen_jit(ctx, llMain)
  
  codegen_ctx_dealloc(ctx)
  return fptr
}

func codegen(ctx: COpaquePointer, value: Procedure) -> COpaquePointer {
  let main = value.blocks.last!
  
  let paramTypes = value.params.map({_, type in return llvmType(ctx, cfgType: type) })
  let resultType = llvmType(ctx, cfgType: cfgType(main))
  
  let procCtx = proc_alloc(ctx, value.name, resultType, paramTypes, CInt(paramTypes.count))
  
  proc_enter_block(procCtx, "result")
  let result = codegen(procCtx, value: main)
  
  proc_set_return(procCtx, result)
  return proc_dealloc_and_complete(procCtx)
}

func codegen(ctx: COpaquePointer, value: SSANode) -> COpaquePointer {
  switch (value) {
  case .AddNum(let lhs, let rhs): return real_add(ctx, codegen(ctx, value: lhs), codegen(ctx, value: rhs))
  }
}

func codegen(ctx: COpaquePointer, value: SSANum) -> COpaquePointer {
  switch (value) {
  case .Real(let x): return real(ctx, x)
  }
}

func llvmType(ctx: COpaquePointer, cfgType: CFGType) -> COpaquePointer {
  switch (cfgType) {
  case .Real: return codegen_type_f64(ctx)
  }
}