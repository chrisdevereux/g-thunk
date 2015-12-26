/** Grammar **/

func module() -> Grammar<ASTModule> {
  /** Keywords **/
  let identifier = consume(whitespace(), surrounding: alpha())
  let arrow = consume(whitespace(), surrounding: "->")
//  let semicolon = consume(whitespace(), surrounding: ";")
  
  let expression = recursive {
    expression -> Grammar<ASTExpression> in
    
    let letBinding = astLetBinding <^> (
      "let " *> identifier <* "=" <*> expression
    )
    
    /** Literals **/
    
    let numberLiteral = astNumber <^> number()
    let ref = astRef <^> alpha()
    
    let functionLiteral = astFunctionDef <^> (
      "(" *> identifier <* ")" <* arrow
      <*> (letBinding<+ <|> accept([]))
      <*> expression
    )
    
    let literal = consume(whitespace(), surrounding:
      numberLiteral
      <|> functionLiteral
      <|> ref
    )
    
    
    /** Function Call **/
    
    let functionCall = consume(whitespace(), surrounding:
      astFunctionCall <^> (
        literal <* "(" <*> expression <* ")"
      )
    )
    
    
    /** Operators **/
    
    let unaryExpression = (
      literal
      <|> functionCall
    )
    
    return grammar(
      unaryExpression
      |> lassoc(["*": astBinaryOp(BinOpType.Mutliply)])
    )
  }
  
  let letBinding = astLetBinding <^> (
    "let " *> identifier <* "=" <*> expression
  )
  
  let topLevelDecl = consume(whitespace(), surrounding:
    letBinding
  )
  
  return grammar(astModule <^> topLevelDecl<+)
}


/** AST Node Constructors **/

func astNumber(string: String) -> ASTExpression {
  return ASTExpression.Real(Double(string)!)
}

func astRef(string: String) -> ASTExpression {
  return ASTExpression.Ref(string)
}

func astFunctionDef(pattern: ((String, [ASTLetBinding]), ASTExpression)) -> ASTExpression {
  let ((param, bindings), value) = pattern
  return ASTExpression.FnDef(param, bindings, value)
}

func astFunctionCall(site: ASTExpression, param: ASTExpression) -> ASTExpression {
  return ASTExpression.FnCall(site, param)
}

func astBinaryOp(op: BinOpType) -> (ASTExpression, ASTExpression) -> ASTExpression {
  return {(lhs, rhs) in return ASTExpression.BinaryOp(op, lhs, rhs)}
}

func astLetBinding(lhs: String, rhs: ASTExpression) -> ASTLetBinding {
  return ASTLetBinding(name: lhs, value: rhs)
}

func astModule(declarations: [ASTLetBinding]) -> ASTModule {
  return ASTModule(declarations: declarations)
}
