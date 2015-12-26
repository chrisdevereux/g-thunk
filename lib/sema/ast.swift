struct ASTModule : Equatable {
  let declarations: [ASTLetBinding]
}

func == (lhs: ASTModule, rhs: ASTModule) -> Bool {
  return lhs.declarations == rhs.declarations
}


struct ASTLetBinding : Equatable {
  let name: String
  let value: ASTExpression
}

func == (lhs: ASTLetBinding, rhs: ASTLetBinding) -> Bool {
  return lhs.name == rhs.name && lhs.value == rhs.value
}


indirect enum ASTExpression : Equatable {
  case Real(Double)
  case Ref(String)
  case FnCall(ASTExpression, ASTExpression)
  case FnDef(String, [ASTLetBinding], ASTExpression)
  case BinaryOp(BinOpType, ASTExpression, ASTExpression)
}

func == (lhs: ASTExpression, rhs: ASTExpression) -> Bool {
  switch (lhs, rhs) {
  case (let .Real(lhs), let .Real(rhs)): return lhs == rhs
  case (let .Ref(lhs), let .Ref(rhs)): return lhs == rhs
  case (let .FnCall(ll, lr), let .FnCall(rl, rr)): return ll == rl && lr == rr
  case (let .FnDef(lparam, lbind, lval), let .FnDef(rparam, rbind, rval)):
    return lparam == rparam && lbind == rbind && lval == rval
  case (let .BinaryOp(ltype, ll, lr), let .BinaryOp(rtype, rl, rr)):
    return ltype == rtype && ll == rl && lr == rr
    
  default: return false
  }
}

enum BinOpType {
  case Mutliply
}