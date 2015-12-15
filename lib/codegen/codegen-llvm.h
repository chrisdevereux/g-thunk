//
//  codegen-llvm.h
//  g-thunk
//
//  Created by Chris Devereux on 15/12/2015.
//
//

#ifndef codegen_llvm_h
#define codegen_llvm_h

#include <unistd.h>

typedef struct Context Context;
typedef struct ProcContext ProcContext;
typedef struct BasicBlock BasicBlock;
typedef struct Value Value;
typedef struct Type Type;
typedef struct Function Function;
typedef double(*JITFn)(void);

Context *codegen_ctx_alloc(void);
void codegen_ctx_dealloc(Context *ctx);
JITFn codegen_jit(Context *ctx, Function *fn);

ProcContext *proc_alloc(Context *ctx, char const *name, Type *resultType, Type *const *paramTypes, int arity);
Function *proc_dealloc_and_complete(ProcContext *ctx);

BasicBlock *proc_enter_block(ProcContext *ctx, char const *name);

void proc_set_return(ProcContext *ctx, Value *value);

/** Types **/

Type *codegen_type_f64(Context *ctx);


/** Value Context **/

Value *real_add(ProcContext *ctx, Value *lhs, Value *rhs);
Value *real(ProcContext *ctx, double value);


#endif /* codegen_llvm_h */
