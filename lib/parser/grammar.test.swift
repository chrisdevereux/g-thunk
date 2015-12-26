import Nimble
import Quick

class GrammarTests: QuickSpec {
  override func spec() {
    describe("Grammar") {
      it("should parse a simple module") {
        let testSyntax = "let main = (time) -> time * 2"
        let expValue = ASTModule(declarations: [
          ASTLetBinding(name: "main", value:
            ASTExpression.FnDef("time", [],
              ASTExpression.BinaryOp(BinOpType.Mutliply,
                ASTExpression.Ref("time"),
                ASTExpression.Real(2)
              )
            )
          )
        ])
        
        expect(module()).to(parse(testSyntax, asValue:expValue, upTo:""))
      }
    }
  }
}