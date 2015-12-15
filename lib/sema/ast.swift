//
//  ast.swift
//  g-thunk
//
//  Created by Chris Devereux on 14/12/2015.
//
//

enum TopLevelDecl {
  case Var(String, Expression)
}

indirect enum Expression {
  case Primitive(Value)
  case BinaryOp(Operator)
}

enum Operator {
  case Plus
}

enum Value {
  case Real(Float64)
}
