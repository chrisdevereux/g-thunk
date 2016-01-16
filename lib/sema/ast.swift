struct ASTModule : Equatable, Hashable {
  let declarations: [ASTLetBinding]
  
  var hashValue: Int {
    get {
      return declarations.prefix(5).reduce(0, combine:hashCombine)
    }
  }
}

func == (lhs: ASTModule, rhs: ASTModule) -> Bool {
  return lhs.declarations == rhs.declarations
}


struct ASTLetBinding : Equatable, Hashable {
  let name: String
  let value: ASTExpression
  
  var hashValue: Int {
    get {
      return 0
    }
  }
}

func == (lhs: ASTLetBinding, rhs: ASTLetBinding) -> Bool {
  return lhs.name == rhs.name && lhs.value == rhs.value
}


indirect enum ASTExpression : Equatable, Hashable {
  case Real(Double)
  case Ref(String)
  case FnCall(ASTExpression, ASTExpression)
  case FnDef(String, [ASTLetBinding], ASTExpression)
  case BinaryOp(BinOpType, ASTExpression, ASTExpression)
  
  var hashValue: Int {
    get {
      switch self {
      case .Real(let val): return val.hashValue
      case .Ref(let id): return id.hashValue
      case .FnCall(let site, let param): return site.hashValue | param.hashValue
      case .FnDef(let param, let bindings, let expr):
        return param.hashValue | bindings.prefix(5).reduce(0, combine:hashCombine) | expr.hashValue
      case .BinaryOp(let type, let lhs, let rhs):
        return type.hashValue | lhs.hashValue | rhs.hashValue
      }
    }
  }
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

func hashCombine<T: Hashable>(hash: Int, decl: T) -> Int {
  return decl.hashValue | hash
}

