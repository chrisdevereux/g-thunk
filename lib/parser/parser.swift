//
//  parser.swift
//  g-thunk
//
//  Created by Chris Devereux on 20/12/2015.
//
//

infix operator <*> { associativity left precedence 130 }
infix operator |> { associativity left precedence 130 }
infix operator <* { associativity left precedence 130 }
postfix operator <+ {}
infix operator <! { associativity left precedence 150 }
infix operator *> { associativity left precedence 130 }
infix operator <|> { associativity left precedence 120 }
infix operator <^> { associativity left precedence 100 }

struct Match<Value> {
  let value: Value
  let next: TokenCursor
}

struct ParseError : ErrorType {
  let message: String
}


/** Token Stream **/

class TokenStream {
  let characters: String.CharacterView

  init(characters: String.CharacterView) {
    self.characters = characters;
  }

  var start: TokenCursor {
    get { return TokenCursor(stream: self, view: characters, index: 0) }
  }
}

struct TokenCursor: Streamable, Equatable, Hashable {
  let stream: TokenStream
  let view: String.CharacterView
  let index: Int

  var hashValue: Int {
    get {
      return unsafeAddressOf(stream).hashValue | index
    }
  }

  var startIndex: String.Index {
    get {
      return view.startIndex
    }
  }
  
  var endIndex: String.Index {
    get {
      return view.endIndex
    }
  }
  
  var stringValue: String {
    get {
      return String(view)
    }
  }

  subscript(range: Range<String.Index>) -> String? {
    get {
      return String(view[range])
    }
  }

  subscript(index: String.Index) -> Character? {
    get {
      return view[index]
    }
  }

  func startsWith(string: String.CharacterView) -> Bool {
    return view.startsWith(string)
  }

  func dropFirst(n: Int = 1) -> TokenCursor {
    return TokenCursor(stream: stream, view: view.dropFirst(n), index: index + n)
  }

  func writeTo<Target : OutputStreamType>(inout target: Target) {
    String(view).writeTo(&target)
  }
}

func == (lhs: TokenCursor, rhs: TokenCursor) -> Bool {
  return lhs.stream === rhs.stream && lhs.index == rhs.index
}


/** Character Class **/


struct CharacterClass {
  let characters: Set<Character>

  func contains(chr: Character) -> Bool {
    return characters.contains(chr)
  }
}

func characters(chrString: String) -> CharacterClass {
  return CharacterClass(characters: Set(chrString.characters))
}

func number() -> CharacterClass {
  return characters("0123456789")
}

func alpha() -> CharacterClass {
  return characters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUFWXYZ")
}

func whitespace() -> CharacterClass {
  return characters(" \n\t")
}


/** Parsers **/

protocol Parser {
  typealias ParseValue
  func parse(input: TokenCursor) throws -> Match<ParseValue>?
}

func parse<Grammar: Parser>(input: String, withGrammar: Grammar) throws -> Match<Grammar.ParseValue>? {
  let tokens = TokenStream(characters: input.characters)
  return try withGrammar.parse(tokens.start)
}


extension String : Parser {
  typealias ParseValue = String

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if (input.startsWith(self.characters)) {
      return Match(value: self, next: input.dropFirst(self.characters.count))

    } else {
      return nil
    }
  }
}


extension CharacterClass: Parser {
  typealias ParseValue = String

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    let range = rangeIn(input)

    return (range.count == 0) ? nil : Match(value: input[range]!, next: input.dropFirst(range.count))
  }

  func rangeIn(input: TokenCursor) -> Range<String.Index> {
    for var i = input.startIndex; i != input.endIndex; i = i.successor() {
      if !contains(input[i]!) {
        return input.startIndex..<i
      }
    }

    return input.startIndex..<input.endIndex
  }
}


struct MappedParser<InputParser: Parser, Value> : Parser {
  typealias ParseValue = Value
  let inputParser: InputParser
  let transform: (InputParser.ParseValue) -> ParseValue

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if let match = try inputParser.parse(input) {
      return Match(value: transform(match.value), next: match.next)

    } else {
      return nil
    }
  }
}

func <^> <P: Parser, T>(transform: (P.ParseValue) -> T, parser: P) -> MappedParser<P, T> {
  return MappedParser(inputParser: parser, transform: transform)
}


struct AcceptParser<T> : Parser {
  typealias ParseValue = T
  let value: T
  
  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    return Match(value: value, next: input)
  }
}

func accept<T>(value: T) -> AcceptParser<T> {
  return AcceptParser(value: value)
}


typealias OptionalStringParser = AlternativeParser<CharacterClass, AcceptParser<String>>

func consume<P: Parser>(characters: CharacterClass, surrounding: P) -> LParser<RParser<OptionalStringParser, P>, OptionalStringParser> {
  let optWS = characters <|> accept("")
  return optWS *> surrounding <* optWS
}


struct PatternParser<LHS: Parser, RHS: Parser> : Parser {
  typealias ParseValue = (LHS.ParseValue, RHS.ParseValue)
  let lhs: LHS
  let rhs: RHS

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if let lRes = try lhs.parse(input) {
      if let rRes = try rhs.parse(lRes.next) {
        return Match(value: (lRes.value, rRes.value), next: rRes.next)
      }
    }

    return nil
  }
}

func <*> <LHS: Parser, RHS: Parser>(lhs: LHS, rhs: RHS) -> PatternParser<LHS, RHS> {
  return PatternParser(lhs: lhs, rhs: rhs)
}


struct LParser<LHS: Parser, RHS: Parser> : Parser {
  typealias ParseValue = LHS.ParseValue
  let lhs: LHS
  let rhs: RHS

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if let lRes = try lhs.parse(input) {
      if let rRes = try rhs.parse(lRes.next) {
        return Match(value: lRes.value, next: rRes.next)
      }
    }

    return nil
  }
}

func <* <LHS: Parser, RHS: Parser>(lhs: LHS, rhs: RHS) -> LParser<LHS, RHS> {
  return LParser(lhs: lhs, rhs: rhs)
}


struct RParser<LHS: Parser, RHS: Parser> : Parser {
  typealias ParseValue = RHS.ParseValue
  let lhs: LHS
  let rhs: RHS

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if let lRes = try lhs.parse(input) {
      if let rRes = try rhs.parse(lRes.next) {
        return Match(value: rRes.value, next: rRes.next)
      }
    }

    return nil
  }
}

func *> <LHS: Parser, RHS: Parser>(lhs: LHS, rhs: RHS) -> RParser<LHS, RHS> {
  return RParser(lhs: lhs, rhs: rhs)
}


struct AlternativeParser<LHS: Parser, RHS: Parser where LHS.ParseValue == RHS.ParseValue> : Parser {
  typealias ParseValue = LHS.ParseValue
  let lhs: MemoizedParser<LHS>
  let rhs: MemoizedParser<RHS>

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    return try lhs.parse(input) ?? rhs.parse(input)
  }
}

func <|> <LHS: Parser, RHS: Parser where LHS.ParseValue == RHS.ParseValue> (lhs: LHS, rhs: RHS) -> AlternativeParser<LHS, RHS> {
  return AlternativeParser(lhs: MemoizedParser(parser: lhs), rhs: MemoizedParser(parser: rhs))
}


struct HomogenousAlternativeParser<P: Parser> : Parser {
  typealias ParseValue = P.ParseValue
  let parsers: [MemoizedParser<P>]
  
  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    for p in parsers {
      if let value = try p.parse(input) {
        return value
      }
    }
    
    return nil
  }
}

func oneOf<P: Parser>(parsers: [P]) -> HomogenousAlternativeParser<P> {
  let memoized = parsers.map{p in return MemoizedParser(parser: p)}
  return HomogenousAlternativeParser(parsers: memoized)
}

func oneOf<P: Parser>(parsers: P...) -> HomogenousAlternativeParser<P> {
  return oneOf(parsers)
}


struct MemoizedParser<P: Parser>: Parser {
  typealias ParseValue = P.ParseValue
  let parser: P
  let cache = Cache<P>()

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    return try cache.get(input, parser: parser)
  }
}

class Cache<P: Parser> {
  var cache = Dictionary<TokenCursor, Match<P.ParseValue>?>()

  func get(input: TokenCursor, parser: P) throws -> Match<P.ParseValue>? {
    if let cached = cache[input] {
      return cached

    } else {
      let value = try parser.parse(input)
      cache[input] = value

      return value
    }
  }
}


struct RepeatParser<P: Parser>: Parser {
  typealias ParseValue = [P.ParseValue]
  let parser: P

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    var value: [P.ParseValue] = []
    var current = input

    while let step = try parser.parse(current) {
      current = step.next
      value.append(step.value)
    }

    if value.count == 0 {
      return nil

    } else {
      return Match(value: value, next: current)
    }
  }
}

postfix func <+ <P: Parser>(parser: P) -> RepeatParser<P> {
  return RepeatParser(parser: parser)
}


/**
  let parser = "'" *> quotedText <* "'" <! "Expected closing quote"
*/
struct RequiredParser<P: Parser> : Parser {
  typealias ParseValue = P.ParseValue
  let parser: P
  let errorMessage: String

  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    if let result = try parser.parse(input) {
      return result

    } else {
      throw ParseError(message: errorMessage)
    }
  }
}

func <! <P: Parser>(parser: P, errorMessage: String) -> RequiredParser<P> {
  return RequiredParser(parser: parser, errorMessage: errorMessage)
}


struct Grammar<T> : Parser {
  typealias ParseValue = T
  let thunk: (input: TokenCursor, rec: Grammar<T>) throws -> Match<ParseValue>?
  
  func parse(input: TokenCursor) throws -> Match<ParseValue>? {
    return try thunk(input: input, rec: self)
  }
}

func grammar<P: Parser>(parser: P) -> Grammar<P.ParseValue> {
  return Grammar(thunk: {(input, rec) in
    return try parser.parse(input)
  });
}

func recursive<T>(parserDef: Grammar<T> -> Grammar<T>) -> Grammar<T> {
  var cached: Grammar<T>?
  var tok: TokenCursor?
  
  func parser(rec: Grammar<T>) -> Grammar<T> {
    if let p = cached {
      return p
      
    } else {
      let p = parserDef(rec)
      cached = p
      return p
    }
  }
  
  func thunk(input: TokenCursor, rec: Grammar<T>) throws -> Match<T>? {
    if let currentTok = tok {
      if currentTok == input {
        throw ParseError(message: "Left-recursive grammar detected!")
      }
    }
    
    let prevTok = tok
    tok = input
    
    defer {
      tok = prevTok
    }
    
    return try parser(rec).parse(input)
  }
  
  return Grammar(thunk: thunk)
}


/**
  let expression = unaryExpression
  |> lassoc(["+": add, "-": subtract])
  |> lassoc("*", multiply) <|> lassoc("/", divide)
*/

protocol ParameterizedParser {
  typealias ParamParser: Parser
  typealias ReturnedParser: Parser

  func apply(param: ParamParser) -> ReturnedParser
}

func |> <ParamParser: Parser, Applied: ParameterizedParser where Applied.ParamParser == ParamParser> (lhs: ParamParser, rhs: Applied) -> Applied.ReturnedParser {
  return rhs.apply(lhs)
}


struct LeftAssocBinaryOpParser<OperandParser: Parser> : ParameterizedParser {
  typealias ParamParser = OperandParser
  typealias ReturnedParser = AlternativeParser<MatchedParser, OperandParser>
  
  typealias Reducer = (ParseValue, ParseValue) -> ParseValue
  typealias ParseValue = OperandParser.ParseValue
  
  typealias MatchedParser = MappedParser<OperationSequenceParser, ParseValue>
  typealias OperationSequenceParser = PatternParser<OperandParser, RepeatParser<RHSParser>>
  typealias RHSParser = HomogenousAlternativeParser<OperationParser>
  typealias OperationParser = PatternParser<String, OperandParser>
  
  let opMap: Dictionary<String, Reducer>
  
  func apply(operand: OperandParser) -> ReturnedParser {
    let options: [OperationParser] = opMap.keys.map{syntax in return syntax <*> operand}
    let sequenceParser: OperationSequenceParser = operand <*> oneOf(options)<+
    
    return (reduce <^> sequenceParser) <|> operand
  }
  
  func reduce(parseOutput: (ParseValue, [(String, ParseValue)])) -> ParseValue {
    let (start, operations) = parseOutput
    
    return operations.reduce(start){(lhs, next) in
      let (keyword, rhs) = next
      let reduce = opMap[keyword]!
      
      return reduce(lhs, rhs)
    }
  }
}

struct RightAssocBinaryOpParser<OperandParser: Parser> : ParameterizedParser {
  typealias ParamParser = OperandParser
  typealias ReturnedParser = AlternativeParser<MatchedParser, OperandParser>
  
  typealias Reducer = (ParseValue, ParseValue) -> ParseValue
  typealias ParseValue = OperandParser.ParseValue
  
  typealias MatchedParser = MappedParser<OperationSequenceParser, ParseValue>
  typealias OperationSequenceParser = PatternParser<RepeatParser<LHSParser>, OperandParser>
  typealias LHSParser = HomogenousAlternativeParser<OperationParser>
  typealias OperationParser = PatternParser<OperandParser, String>
  
  let opMap: Dictionary<String, Reducer>
  
  func apply(operand: OperandParser) -> ReturnedParser {
    let options: [OperationParser] = opMap.keys.map{syntax in return operand <*> syntax}
    let sequenceParser: OperationSequenceParser = oneOf(options)<+ <*> operand
    
    return (reduce <^> sequenceParser) <|> operand
  }
  
  func reduce(parseOutput: ([(ParseValue, String)], ParseValue)) -> ParseValue {
    let (operations, start) = parseOutput
    
    return operations.reverse().reduce(start){(rhs, next) in
      let (lhs, keyword) = next
      let reduce = opMap[keyword]!
      
      return reduce(lhs, rhs)
    }
  }
}

func lassoc<OperandParser: Parser>(operations: Dictionary<String, LeftAssocBinaryOpParser<OperandParser>.Reducer>) -> LeftAssocBinaryOpParser<OperandParser> {
  return LeftAssocBinaryOpParser(opMap: operations)
}

func rassoc<OperandParser: Parser>(operations: Dictionary<String, RightAssocBinaryOpParser<OperandParser>.Reducer>) -> RightAssocBinaryOpParser<OperandParser> {
  return RightAssocBinaryOpParser(opMap: operations)
}

//func <|> <ParamP: Parser, ValueP: Parser>(lhs: ParserBind<ParamP, ValueP>, rhs: ParserBind<ParamP, ValueP>) -> ParserBind<ParamP, ValueP> {
//  return ParserBind{param in
//    return lhs.transform(param) <|> rhs.transform(param)
//  }
//}
