//
//  ast.test.swift
//  g-thunk
//
//  Created by Chris Devereux on 14/12/2015.
//
//

import Quick
import Nimble

class ASTTests : QuickSpec {
  override func spec() {
    describe("When a function is JIT-ed") {
      it("should return the function result") {
        let fnPtr = codegen_prog(Program(
          procedures: [
            Procedure(
              name: "TestProc",
              params: [],
              blocks: [
                SSANode.AddNum(SSANum.Real(10), SSANum.Real(12))
              ]
            )
          ],
          entry: 0
        ))
        
        expect(fnPtr()).to(equal(22.0))
      }
    }
  }
}
