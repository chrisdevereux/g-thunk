#include "Syntax.hpp"

using namespace parse;
using namespace ast;

namespace {
  template <typename Action>
  Grammar expression(Action out);
  
  auto variableHead = exactly('_') || exactly('\'') || lowercase;
  auto parens = range('(', ')') || range('[', ']') || range('{', '}');
  
  template <typename Action>
  auto variableName(Action const &out) {
    return identifierString(variableHead, variableHead || uppercase || digitChar, out);
  }
  
  template <typename Action>
  auto operatorName(Action const &out) {
    return identifierString(printableChar && !variableHead && !parens, printableChar && !parens, out);
  }
  
  
  /** Atoms **/
  
  template <typename Action>
  auto parenthesizedExpression(Action out) {
    return [=](State const &state) -> Result {
      return state
      >> match("(")
      >> optionalWhitespace
      >> expression(out)
      >> optionalWhitespace
      >> match(")")
      ;
    };
  }
  
  template <typename Action>
  auto scalarLiteral(Action out) {
    return [=](State const &state) -> Result {
      auto result = state.create<Scalar>();
      
      return state
      >> real(receive(&result->value))
      >> emit(&result, out)
      ;
    };
  }
  
  template <typename Action>
  auto identifier(Action out) {
    return [=](State const &state) -> Result {
      auto result = state.create<Identifier>();
      
      return state
      >> variableName(receive(&result->value))
      >> emit(&result, out)
      ;
    };
  }
  
  
  /** Function application **/
  
  template <typename Action>
  auto applyTerm(Action out) {
    return [=](State const &state) -> Result {
      return state >> scalarLiteral(out)
      ?: state >> identifier(out)
      ?: state >> parenthesizedExpression(out)
      ;
    };
  }
  
  template <typename Action>
  auto apply(Action out) {
    return [=](State const &state) -> Result {
      auto result = state.create<Apply>(state.arena);
      
      return state
      >> applyTerm(receive(&result->function))
      >> whitespace
      >> delimited(applyTerm(collect(&result->params)), whitespace)
      >> emit(&result, out)
      ;
    };
  }
  
  
  /** Binary Operands **/
  
  template <typename Action>
  Grammar binaryOperand(Action out) {
    return [=](State const &state) -> Result {
      return state >> apply(out)
      ?: state >> applyTerm(out)
      ;
    };
  }
  
  template <typename Action>
  Grammar binaryOpTerm(Action out) {
    return [=](State const &state) -> Result {
      OperatorSequence::Term result;
      
      return state
      >> operatorName(receive(&result.symbol))
      >> optionalWhitespace
      >> binaryOperand(receive(&result.operand))
      >> emit(&result, out)
      ;
    };
  }
  
  template <typename Action>
  Grammar binaryOpSequence(Action out) {
    return [=](State const &state) -> Result {
      auto result = state.create<OperatorSequence>(state.arena);
      
      return state
      >> binaryOperand(receive(&result->lhs))
      >> optionalWhitespace
      >> delimited(binaryOpTerm(collect(&result->terms)), optionalWhitespace)
      >> emit(&result, out)
      ;
    };
  }
  
  
  /** Any expression **/
  
  template <typename Action>
  Grammar expression(Action out) {
    return [=](State const &state) -> Result {
      return state >> binaryOpSequence(out)
      ?: state >> binaryOperand(out)
      ;
    };
  }
  
  template <typename Action>
  auto topLevelFunction(Action out) {
    return [=](State const &state) -> Result {
      auto fn = state.create<Function>(state.arena);
      Declaration result;
      result.value = fn;
      
      return state
      >> variableName(receive(&result.name))
      >> spaces
      >> optional(delimited(variableName(collect(&fn->params)), spaces))
      >> optional(spaces)
      >> match("=")
      >> optionalWhitespace
      >> expression(receive(&fn->value))
      >> optionalWhitespace
      >> match(";")
      >> emit(&result, out)
      ;
    };
  }
  
  template <typename Action>
  auto topLevelDecl(Action out) {
    return topLevelFunction(out);
  }
}

namespace syntax {
  Grammar expression(GenericAction<Expression *> out) {
    return ::expression(out);
  }
  
  Grammar module(GenericAction<Module> out) {
    return [=](State const &state) -> Result {
      Module result(state.arena);
      
      return state
      >> optionalWhitespace
      >> delimited(topLevelDecl(collect(&result.declarations)), newline)
      >> optionalWhitespace
      >> emit(&result, out)
      ;
    };
  }
}

