/**
  Program Structure
**/

struct Program {
  let procedures: [Procedure]
  let entry: Int
}

struct Procedure {
  let name: String
  let params: [(String, CFGType)]
  let blocks: [SSANode]
}


/** Types **/

enum CFGType {
  case Real
}


/** SSA Graph **/

enum SSANode {
  case AddNum(SSANum, SSANum)
}

func cfgType(node: SSANode) -> CFGType {
  switch (node) {
  case .AddNum(let lhs, _): return cfgType(lhs)
  }
}


/** Primitives **/

enum SSAValue {
  case Num(SSANum)
}

enum SSANum {
  case Real(Float64)
}

func cfgType(val: SSAValue) -> CFGType {
  switch (val) {
  case .Num(let x): return cfgType(x)
  }
}

func cfgType(val: SSANum) -> CFGType {
  switch (val) {
  case .Real: return CFGType.Real
  }
}
