import Quick
import Nimble
import SwiftCheck

protocol TestableParser: Arbitrary, Parser {
  static func arbitraryOutput() -> Gen<Self.ParseValue>
  static func arbitraryInputs(output: Self.ParseValue) -> Gen<String>
}

extension CharacterClass: Arbitrary {
  static var arbitrary: Gen<CharacterClass> {
    get {
      return String.arbitrary.map{ CharacterClass(characters: Set($0.characters)) }
    }
  }
}


struct ValidOutput<ParserType: TestableParser>: Arbitrary {
  let parser: ParserType
  let input: String
  let output: ParserType.ParseValue
  
  static var arbitrary: Gen<ValidOutput<ParserType>> {
    get {
      return Gen
        .zip(ParserType.arbitrary, ParserType.arbitraryOutput())
        .flatMap{ (parser: ParserType, output: ParserType.ParseValue) in
          return ParserType
            .arbitraryInputs(output)
            .map{ (input : String) in
              ValidOutput(parser: parser, input: input, output: output)
            }
        }
    }
  }
}


class ParserTests: QuickSpec {
  override func spec() {
    describe("Character Class") {
      it("should match string containing matched characters") {
        expect(characters("12345")).to(parse("44321zz1", asValue: "44321", upTo: "zz1"))
      }
      
      it("should reject other characters") {
        expect(characters("12345")).to(reject("zz1"))
      }
      
      it("should reject at EOF") {
        expect(characters("12345")).to(reject(""))
      }
    }
    
    describe("String") {
      it("should match exact string") {
        expect("12345").to(parse("123451", asValue: "12345", upTo: "1"))
      }
      
      it("should reject partial match") {
        expect("12345").to(reject("1234"))
      }
      
      it("should reject at EOF") {
        expect("12345").to(reject(""))
      }
    }
    
    describe("consume") {
      it("should consume class") {
        expect(consume(whitespace(), surrounding: "foo")).to(parse("   foo   z", asValue: "foo", upTo: "z"))
      }
      
      it("should not require class") {
        expect(consume(whitespace(), surrounding: "foo")).to(parse("fooz", asValue: "foo", upTo: "z"))
      }
    }
    
    describe("MappedParser") {
      it("should map values") {
        func uppercase(x: String) -> String { return x.uppercaseString }
        expect(uppercase <^> "x").to(parse("x1", asValue: "X", upTo: "1"))
      }
    }
    
    describe("PatternParser") {
      it("should match exact pattern") {
        expect("x" <*> "y").to(parse("xy1", asValue: ("x", "y"), upTo: "1"))
      }
      
      it("should reject partial match") {
        expect("x" <*> "y").to(reject("x"))
      }
    }
    
    describe("LParser") {
      it("should match exact pattern") {
        expect("x" <* "y").to(parse("xy1", asValue: "x", upTo: "1"))
      }
      
      it("should reject partial match") {
        expect("x" <* "y").to(reject("x"))
      }
    }
    
    describe("RParser") {
      it("should match exact pattern") {
        expect("x" *> "y").to(parse("xy1", asValue: "y", upTo: "1"))
      }
      
      it("should reject partial match") {
        expect("x" *> "y").to(reject("x"))
      }
    }
    
    describe("AlternativeParser") {
      it("should prefer first option") {
        expect("x" <|> "xx").to(parse("xxy", asValue: "x", upTo: "xy"))
      }
      
      it("should fall through to second option") {
        expect("xy" <|> "xx").to(parse("xxy", asValue: "xx", upTo: "y"))
      }
      
      it("should reject if no matches") {
        expect("xy" <|> "xx").to(reject("z"))
      }
    }
    
    describe("HomogenousAlternativeParser") {
      it("should prefer first option") {
        expect(oneOf("x", "xx")).to(parse("xxy", asValue: "x", upTo: "xy"))
      }
      
      it("should fall through to second option") {
        expect(oneOf("xy", "xx")).to(parse("xxy", asValue: "xx", upTo: "y"))
      }
      
      it("should reject if no matches") {
        expect(oneOf("xy", "xx")).to(reject("z"))
      }
    }
    
    describe("RepeatParser") {
      it("should parse multiple instances") {
        expect("x"<+).to(parse("xxxxy", asValue: ["x", "x", "x", "x"], upTo: "y"))
      }
      
      it("should reject if not matched") {
        expect("x"<+).to(reject("z"))
      }
    }
    
    describe("RequiredParser") {
      it("should propagate acceptances") {
        expect("x" <! "Expected x").to(parse("xy", asValue: "x", upTo: "y"))
      }
      
      it("should reject if not matched") {
        expect("x" <! "Expected x").to(parse("y", asError: "Expected x"))
      }
    }
    
    describe("RecursiveParser") {
      it("should parse recursively") {
        func exclaim(str: String) -> String { return "\(str)!" }
        
        let recursiveExpr = recursive {(expr: Grammar<String>) in
          return grammar((exclaim <^> "f(" *> expr <* ")") <|> number())
        }
        
        expect(recursiveExpr).to(parse("f(f(f(123)))z", asValue: "123!!!", upTo: "z"))
      }
      
      it("should throw for left-recursive grammar") {
        func exclaim(str: String) -> String { return "\(str)!" }
        
        let recursiveExpr = recursive {(expr: Grammar<String>) in
          return expr
        }
        
        expect(recursiveExpr).to(parse("f(f(f(123)))z", asError: "<TODO>"))
      }
    }
    
    describe("LeftAssociativeBinaryOpParser") {
      it("should partition into operators, respecting precedence") {
        let value = opVal <^> characters("1234567890")
        let expr = value
          |> lassoc(["*": binOp("mul")])
          |> lassoc(["+": binOp("add"), "-": binOp("sub")])
        
        let expected =
        binOp("add")(
          binOp("sub")(
            binOp("add")(
              opVal("1"),
              binOp("mul")(
                opVal("2"),
                opVal("2")
              )
            ),
            opVal("3")
          ),
          opVal("4")
        )
        
        expect(expr).to(parse("1+2*2-3+4z", asValue:expected, upTo:"z"))
      }
    }
    
    describe("RightAssociativeBinaryOpParser") {
      it("should partition into operators, respecting precedence") {
        let value = opVal <^> characters("1234567890")
        let expr = value
          |> rassoc(["==": binOp("eq"), "=": binOp("set")])
          |> rassoc([",": binOp("exps")])
        
        let expected =
        binOp("exps")(
          binOp("set")(
            opVal("1"),
            binOp("eq")(
              opVal("4"),
              binOp("eq")(
                opVal("3"),
                opVal("5")
              )
            )
          ),
          opVal("5")
        )
        
        expect(expr).to(parse("1=4==3==5,5z", asValue:expected, upTo:"z"))
      }
    }
  }
}


/** Helpers **/

private indirect enum TestBinaryExpr : Equatable, Streamable {
  case BinOp(String, TestBinaryExpr, TestBinaryExpr)
  case Atom(String)
  
  func writeTo<Target : OutputStreamType>(inout target: Target) {
    switch (self) {
    case .BinOp(let op, let lhs, let rhs):
      var lstr = ""
      var rstr = ""
      lhs.writeTo(&lstr)
      rhs.writeTo(&rstr)
      target.write("(\(op) \(lstr) \(rstr))")
      
    case .Atom(let val): target.write(val)
    }
  }
}

private func ==(lhs: TestBinaryExpr, rhs: TestBinaryExpr) -> Bool {
  switch (lhs, rhs) {
  case (let .BinOp(lo, ll, lr), let .BinOp(ro, rl, rr)):
    return lo == ro && ll == rl && lr == rr
  case (let .Atom(lv), let .Atom(rv)):
    return lv == rv
  default:
    return false
  }
}

private func binOp(tag: String) -> (TestBinaryExpr, TestBinaryExpr) -> TestBinaryExpr {
  return {a, b in return TestBinaryExpr.BinOp(tag, a, b)}
}

private func opVal(val: String) -> TestBinaryExpr {
  return TestBinaryExpr.Atom(val)
}


func parse<T, P: Parser where T: Equatable, P.ParseValue == T>(string: String, asValue: T, upTo: String? = nil) -> MatcherFunc<P> {
  return MatcherFunc {parseParam, failureMessage in
    let grammar = try parseParam.evaluate()
    failureMessage.expected = "expected \(grammar!)"
    failureMessage.actualValue = nil
    
    let output = try parse(string, withGrammar:grammar!)
    
    if let match = output {
      failureMessage.actualValue = String(match.value)
      
      if match.value != asValue {
        failureMessage.postfixMessage = "parse value \(asValue)"
        return false
      }
      
      if let expectedNext = upTo {
        if expectedNext != match.next.stringValue {
          failureMessage.postfixMessage = "consume upto \(expectedNext)"
          return false
        }
      }
      
      return true
      
    } else {
      failureMessage.postfixMessage = "match"
      return false
    }
  }
}

func parse<T, P: Parser where T: Equatable, P.ParseValue == [T]>(string: String, asValue: [T], upTo: String? = nil) -> MatcherFunc<P> {
  return MatcherFunc {parseParam, failureMessage in
    let grammar = try parseParam.evaluate()
    failureMessage.expected = "expected \(grammar!)"
    failureMessage.actualValue = nil
    
    let output = try parse(string, withGrammar:grammar!)
    
    if let match = output {
      failureMessage.actualValue = String(match.value)
      
      if match.value != asValue {
        failureMessage.postfixMessage = "parse value \(asValue)"
        return false
      }
      
      if let expectedNext = upTo {
        if expectedNext != match.next.stringValue {
          failureMessage.postfixMessage = "consume upto \(expectedNext)"
          return false
        }
      }
      
      return true
      
    } else {
      failureMessage.postfixMessage = "match"
      return false
    }
  }
}

func parse<T1: Equatable, T2: Equatable, P: Parser where P.ParseValue == (T1, T2)>(string: String, asValue: (T1, T2), upTo: String? = nil) -> MatcherFunc<P> {
  return MatcherFunc {parseParam, failureMessage in
    let grammar = try parseParam.evaluate()
    failureMessage.expected = "expected \(grammar!)"
    failureMessage.actualValue = nil
    
    let (exp1, exp2) = asValue
    let output = try parse(string, withGrammar:grammar!)
    
    if let match = output {
      let (actual1, actual2) = match.value
      failureMessage.actualValue = String(match)
      
      if actual1 != exp1 || actual2 != exp2 {
        failureMessage.postfixMessage = "parse value \(asValue)"
        return false
      }
      
      if let expectedNext = upTo {
        if expectedNext != match.next.stringValue {
          failureMessage.postfixMessage = "consume upto \(expectedNext)"
          return false
        }
      }
      
      return true
      
    } else {
      failureMessage.postfixMessage = "match"
      return false
    }
  }
}

func reject<P: Parser>(string: String) -> MatcherFunc<P> {
  return MatcherFunc {parseParam, failureMessage in
    let grammar = try parseParam.evaluate()
    failureMessage.expected = "expected \(grammar!)"
    
    let output = try parse(string, withGrammar:grammar!)
    
    if output == nil {
      return true
      
    } else {
      failureMessage.postfixMessage = "not match"
      return false
    }
  }
}

func parse<P: Parser>(string: String, asError: String) -> MatcherFunc<P> {
  return MatcherFunc {parseParam, failureMessage in
    let grammar = try parseParam.evaluate()
    failureMessage.expected = "expected \(grammar!)"
    
    do {
      try parse(string, withGrammar:grammar!)
      failureMessage.postfixMessage = "raise `\(asError)`"
      return false
      
    } catch {
      return true
    }
  }
}
