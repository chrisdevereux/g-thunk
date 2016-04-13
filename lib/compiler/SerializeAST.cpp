#include "SerializeAST.hpp"
#include "StringifyUtil.hpp"
#include "AST.hpp"

namespace ast {
  using namespace parse;

  // Expression parser type
  template <typename Action>
  Grammar expressionTree(Action const &result);
  
  // AST stringifier type
  struct ASTStringifier : Expression::Visitor {
    ASTStringifier(std::ostream &str) : stringify(str) {}

    Stringifier stringify;
    
    // Expression stringifiers
    virtual void acceptScalar(Scalar const *s);
    virtual void acceptIdentifier(Identifier const *s);
    virtual void acceptFunction(Function const *s);
    virtual void acceptApply(Apply const *s);
    virtual void acceptLexicalScope(LexicalScope const *s);
    
    // Non-expression stringifiers
    void acceptDeclaration(Declaration const *s);
    void acceptModule(Module const *s);
  };
  
  
  /** Declaration **/
  
  // Parse
  template <typename Action>
  auto declaration(Action const &out) {
    return taggedSExp("let", [=](State const &state) -> Result {
      Declaration result(state.arena);
      
      return state
      >> identifierString(receive(&result.name))
      >> whitespace
      >> expressionTree(receive(&result.value))
      >> emit(&result, out) >> log("declaration")
      ;
    });
  }
  
  // Stringify
  void ASTStringifier::acceptDeclaration(const ast::Declaration *s) {
    stringify.begin("let");
    stringify.atom(s->name);
    stringify.compound(s->value, this);
    stringify.end();
  }
  
  
  /** Lexical scope **/
  
  // Parse
  template <typename Action>
  auto lexicalScope(Action const &out) {
    return sExp([=](State const &state) -> Result {
      LexicalScope *result = state.create<LexicalScope>(state.arena);
      
      return state
      >> delimited(declaration(collect(&result->bindings)), whitespace)
      >> whitespace
      >> expressionTree(receive(&result->value))
      >> emit(&result, out) >> log("scope")
      ;
    });
  }
  
  // Stringify
  void ASTStringifier::acceptLexicalScope(const ast::LexicalScope *s) {
    stringify.begin();
    stringify.each(s->bindings, this, &ASTStringifier::acceptDeclaration);
    stringify.compound(s->value, this);
    stringify.end();
  }
  
  
  /** Function definition **/
  
  // Parse
  template <typename Action>
  auto functionDef(Action const &out) {
    return taggedSExp("\\", [=](State const &state) -> Result {
      Function *result = state.create<Function>(state.arena);
      
      return state
      >> delimited(identifierString(collect(&result->params)), whitespace)
      >> whitespace
      >> expressionTree(receive(&result->value))
      >> emit(&result, out) >> log("fndef")
      ;
    });
  }
  
  // Stringify
  void ASTStringifier::acceptFunction(const ast::Function *s) {
    stringify.begin("\\");
    stringify.each(s->params);
    stringify.compound(s->value, this);
    stringify.end();
  }
  
  
  /** Function Call **/
  
  // Parse
  template <typename Action>
  auto functionCall(Action const &out) {
    return sExp([=](State const &state) -> Result {
      Apply *result = state.create<Apply>(state.arena);
      
      return state
      >> expressionTree(receive(&result->function))
      >> whitespace
      >> delimited(expressionTree(collect(&result->params)), whitespace)
      >> emit(&result, out) >> log("fncall")
      ;
    });
  }
  
  // Stringify
  void ASTStringifier::acceptApply(const ast::Apply *s) {
    stringify.begin();
    stringify.compound(s->function, this);
    stringify.each(s->params, this);
    stringify.end();
  }
  
  
  /** Identifier **/
  
  // Parse
  template <typename Action>
  auto identifier(Action const &out) {
    return [=](State const &state) -> Result {
      Identifier *result = state.create<Identifier>(state.arena);
      
      return state
      >> identifierString(receive(&result->value))
      >> emit(&result, out) >> log("ident")
      ;
    };
  }
  
  // Stringify
  void ASTStringifier::acceptIdentifier(const ast::Identifier *s) {
    stringify.atom(s->value);
  }
  
  
  /** Scalar **/
  
  // Parse
  template <typename Action>
  auto scalar(Action const &out) {
    return [=](State const &state) -> Result {
      Scalar *result = state.create<Scalar>();
      
      return state
      >> real(receive(&result->value))
      >> emit(&result, out) >> log("scalar")
      ;
    };
  }
  
  // Stringify
  void ASTStringifier::acceptScalar(const ast::Scalar *s) {
    stringify.atom(s->value);
  }
  
  
  /** Expression Variant **/
  
  // Internal version
  template <typename Action>
  Grammar expressionTree(Action const &result) {
    return [=](State const &state) -> Result {
      return state >> log("try scope...") >> lexicalScope(result)
      ?: state >> log("try fndef...") >> functionDef(result)
      ?: state >> log("try fncall...") >> functionCall(result)
      ?: state >> log("try ident...") >> identifier(result)
      ?: state >> log("try scalar...") >> scalar(result);
    };
  }
  
  // Exported top-level version with action type erased
  Grammar unserialize::expression(GenericAction<Expression const *> out) {
    return expressionTree(out);
  }
  
  
  /** Module **/
  
  // Parse
  Grammar unserialize::module(GenericAction<Module> out) {
    return [=](State const &state) -> Result {
      Module result = Module(state.arena);
      
      return state
      >> optionalWhitespace
      >> delimited(declaration(collect(&result.declarations)), optionalWhitespace)
      >> optionalWhitespace
      >> emit(&result, out)
      ;
    };
  }
  
  // Stringify
  void ASTStringifier::acceptModule(const ast::Module *s) {
    stringify.each(s->declarations, this, &ASTStringifier::acceptDeclaration);
  }
  
  
  /** Stringify exports **/
  
  namespace serialize {
    std::ostream &operator<<(std::ostream &str, Expression const &rhs) {
      ASTStringifier serializer(str);
      rhs.visit(&serializer);
      
      return str;
    }
    
    std::ostream &operator<<(std::ostream &str, Module const &rhs) {
      ASTStringifier serializer(str);
      serializer.acceptModule(&rhs);
      
      return str;
    }
    
    std::ostream &operator<<(std::ostream &str, Declaration const &rhs) {
      ASTStringifier serializer(str);
      serializer.acceptDeclaration(&rhs);
      
      return str;
    }
  }
}
