#pragma once

#include "Arena.hpp"
#include "Symbol.hpp"

#include <experimental/optional>
#include <functional>
#include <string>
#include <sstream>
#include <iostream>

namespace parse {
  /**
   Composable functions for PEG-like recursive descent parsing.
   
   A parser is a function from input parse state to output parse state.
   It either succeeds and returns the next state, or fails and returns
   nothing.
   
   When a parser succeeds it may emit an event. The core parse functions
   have no concept of a parse value -- instead the factory function may
   accept a callable action object with the signature void(T const &).
   
   Parsers may be composed using the >> operator.
   
   This runs parsers in sequence, using the output state of the lhs parser
   as the input state of the rhs parser, aborting if any parser is rejected.
   
   Eg: 
      parse::State myParser(parse::State const &inputState) {
        return inputState >> parser1 >> parser2 >> parser3;
      }
   */
  
  
  // Represents the current parse state.
  struct State {
    typedef std::shared_ptr<std::vector<std::string>> ErrorList;
    ErrorList errors;
    Arena::string const *input;
    size_t offset;
    Arena *arena;
    
    // Initialize the parse state from an input stream.
    static State read(std::istream &input, Arena *arena) {
      if (!input) {
        throw "Invalid input stream";
      }
      
      Arena::string *string = arena->create<Arena::string>(arena->allocator<char>());
      
      input.seekg(0, input.end);
      string->resize(input.tellg());
      input.seekg(0, input.beg);
      
      char *ptr = (&*string->begin());
      input.read(ptr, string->size());
      
      return State(string, arena, 0);
    }
    
    State(Arena::string const *input_, Arena *arena_, size_t offset_, ErrorList errors_ = std::make_shared<std::vector<std::string>>())
    : errors(errors_)
    , input(input_)
    , offset(offset_)
    , arena(arena_)
    {}
    
    // STL allocator for the current arena
    template <typename T>
    Arena::Allocator<T> allocator() const {
      return arena->allocator<T>();
    }
    
    // Allocate a new object in the current arena
    template <typename T, typename ...Params>
    T *create(Params ...params) const {
      return arena->create<T>(params...);
    }
    
    // Remaining input stream length
    inline size_t size() const {
      return input->size() - offset;
    }
    
    // Get a character from the input stream
    inline char get(size_t i) const {
      return (*input)[i + offset];
    }
    
    // Get the entire remaining input stream
    inline char const *get() const {
      return input->data() + offset;
    }
    
    // Return the next parse state, consuming `count` characters
    inline State advance(size_t count) const {
      return State(input, arena, offset + count, errors);
    }
    
    // Return the line # of the current cursor
    inline size_t lineNo() const {
      size_t line = 1;
      
      for (size_t i = 0; i < offset; ++i) {
        if (get(i) == '\n') ++line;
      }
      
      return line;
    }
  };
  
  // Parser return value
  using Result = std::experimental::optional<State>;
  auto const reject = std::experimental::nullopt;
  
  // Parser concept
  template <typename T>
  using is_parser = std::is_same<std::result_of<T(State const &)>, Result>;
  
  // Exportable parser type
  using Grammar = std::function<Result(State const &)>;
  
  // Convenience function, applying a parser to an input stream
  // and returning true on success, or false on failure
  template <typename Parser>
  bool read(std::istream &str, Arena *arena, Parser const &parser, std::vector<std::string> *errors = nullptr) {
    State state(State::read(str, arena));
    
    if (parser(state)) {
      return true;
      
    } else {
      if (errors) *errors = *state.errors;
      return false;
    }
  }
  
  
  /**
   Predicates
   
   Function object with signature bool(char) defining a character set.
   **/
  
  // Match only the character identified by `chr`
  inline auto exactly(char lhs) {
    return [=](char rhs){ return lhs == rhs; };
  }
  
  // Match only characters in the range `min`..`max`
  inline auto range(char min, char max) {
    return [=](char chr) -> bool {
      return chr >= min && chr <= max;
    };
  }
  
  namespace operators {
    // Union of lhs & rhs character sets
    template <typename LHS, typename RHS>
    auto operator||(const LHS &lhs, const RHS &rhs) {
      return [=](char chr) -> bool {
        return lhs(chr) || rhs(chr);
      };
    }
    
    // Inverse character set
    template <typename LHS>
    auto operator!(const LHS &lhs) {
      return [=](char chr) -> bool {
        return !lhs(chr);
      };
    }
    
    // Intersection character set
    template <typename LHS, typename RHS>
    auto operator&&(const LHS &lhs, const RHS &rhs) {
      return [=](char chr) -> bool {
        return lhs(chr) && rhs(chr);
      };
    }
  }
  
  /**
   Actions
   
   Function object with signature void(T const &), passed to a parser
   on construction defining how to handle a parse value.
   **/
  
  
  // Exportable action type for top-level parsers
  template <typename T>
  using GenericAction = std::function<void(T const &)>;
  
  // Reset a std::unique ptr to hold parse value
  template <typename T>
  auto receivePointerValue(std::unique_ptr<T> *target) {
    return [=](T const &val) {
      target->reset(new T(val));
    };
  }
  
  // Set a single variable to the parse value
  template <typename T>
  auto receive(T *target) {
    return [=](T const &val) {
      *target = val;
    };
  }
  
  // Set flags on target
  template <typename T>
  auto receiveFlags(T *target) {
    return [=](T val) {
      *target = *target | val;
    };
  }
  
  // Append each received parse value to a collection
  template <typename Collection>
  auto collect(Collection *target) {
    typedef typename Collection::value_type T;
    
    return [=](T const &val) {
      target->push_back(val);
    };
  }
  
  // Append each received parse value char to a string
  inline auto buildString(Arena::string *target) {
    return [=](char const &val) {
      target->append(1, val);
    };
  }
  
  // Append each received parse value char to a string
  inline auto receiveSymbol(Symbol *target) {
    return [=](Arena::string const &val) {
      *target = Symbol::get(val);
    };
  }
  
  // Null action (does nothing)
  template <typename T>
  inline auto noop() {
    return [=](T val) {};
  }
  
  
  /**
   Atoms
  
   Primitive parser components.
  **/
  
  // Match a single character using a predicate and emit the parsed character
  template <typename Predicate, typename Action = decltype(noop<char>())>
  auto match(Predicate const &predicate, Action const &action = noop<char>()) {
    return [=](State const &state) -> Result {
      if (state.size() != 0 && predicate(state.get(0))) {
        action(state.get(0));
        return state.advance(1);
        
      } else {
        return reject;
      }
    };
  }
  
  // Match an exact string. Does not emit an action.
  inline auto match(char const *string) {
    size_t len = strlen(string);
    
    return [=](State const &state) -> Result {
      if (strncmp(state.get(), string, len) == 0) {
        return state.advance(len);
        
      } else {
        return reject;
      }
    };
  }
  
  // Set a constant value
  template <typename T>
  auto set(T value, T *target) {
    return [=](State const &state) -> Result {
      *target = value;
      return state;
    };
  }
  
  // Read value and emit to the provided action.
  // Use this to emit a value at the end of a sequence of parsers.
  template <typename T, typename Action>
  auto emit(T const *value, Action const &target) {
    return [=](State const &state) -> Result {
      target(*value);
      return state;
    };
  }
  
  // Emit constant value for the provided action.
  // Use this to emit a value at the end of a sequence of parsers.
  template <typename T, typename Action>
  auto emitValue(T const value, Action const &target) {
    return [=](State const &state) -> Result {
      target(value);
      return state;
    };
  }
  
  // Inject a side-effect into a sequence of parsers.
  template <typename Fn>
  auto inject(Fn const &fn) {
    return [&](State const &state) -> Result {
      fn();
      return state;
    };
  }
  
  // Log a status message if this parser is ever called and
  // TEMPO_PARSER_LOGGING is defined
  inline auto log(char const *msg) {
    return [=](State const &state) -> Result {
#ifdef TEMPO_PARSER_LOGGING
      std::cout << msg << std::endl;
#endif
      return state;
    };
  }
  
  // Log an error to the parse state's error list
  inline Result fail(State state, char const *msg) {
    std::stringstream err;
    err << "Parse error (line " << state.lineNo() << "):\n"
    << "expected " << msg;
    
    state.errors->push_back(err.str());
    return reject;
  }
  
  
  
  /** Combinators **/
  
  // Try the parser, continuing with the previous state if it fails
  template <typename Parser>
  auto optional(Parser const &parser) {
    std::enable_if<is_parser<Parser>::value>();
    
    return [=](State const &state) -> Result {
      return parser(state) ?: state;
    };
  }
  
  // Log an error if the parse fails
  template <typename Parser>
  inline auto require(char const *msg, Parser const &parser) {
    std::enable_if<is_parser<Parser>::value>();
    
    return [=](State const &state) -> Result {
      return parser(state) ?: fail(state, msg);
    };
  }
  
  // Repeatedly call the parser until it fails, succeeding if the parser matched
  // at least once.
  template <typename Parser>
  auto repeat(Parser const &content) {
    return [=](State const &initialState) -> Result {
      std::enable_if<is_parser<Parser>::value>();
      
      Result step;
      State nextState = initialState;
      
      bool match = false;
      
      while (( step = content(nextState) )) {
        nextState = *step;
        match = true;
      }
      
      if (match) {
        return nextState;
        
      } else {
        return reject;
      }
    };
  }
  
  
  /** Sequencing **/
  
  namespace operators {
    // Feed an input state into a sequence of parsers
    template <typename Parser>
    Result operator>> (State const &input, Parser const &parser) {
      std::enable_if<is_parser<Parser>::value>();
      
      return parser(input);
    }
    
    // Pipe a parse result into the next parser, rejecting if the previous
    // parser failed
    template <typename Parser>
    Result operator>> (Result const &input, Parser const &parser) {
      std::enable_if<is_parser<Parser>::value>();
      
      if (input) {
        return parser(*input);
        
      } else {
        return input;
      }
    }
    
    // Parser composition
    template <typename LHS, typename RHS>
    auto operator>> (LHS const &lhs, RHS const &rhs) {
      std::enable_if<is_parser<LHS>::value>();
      std::enable_if<is_parser<RHS>::value>();
      
      return [=](State const &state) -> Result {
        return state >> lhs >> rhs;
      };
    }
  }
  
  
  
  
  /** Conveniences **/
  
  namespace {
    auto digitChar = range('0', '9');
    auto whitespaceChar = range(0x01, ' ');
    auto printableChar = range('!', CHAR_MAX);
    auto parenChar = range('(', ')');
    
    auto spaces = repeat(match(exactly(' ')));
    auto newline = repeat(match(exactly('\n')));
    auto whitespace = repeat(match(whitespaceChar));
    auto optionalWhitespace = optional(whitespace);
    
    // Match the keyword, raising an error if it is not matched
    inline auto requiredMatch(char const *string) {
      return require(string, match(string));
    }
  }
  
  // Match one or more of `member`, separated by `delimiter`
  template <typename Parser, typename Delimiter>
  auto delimited(Parser member, Delimiter delimiter) {
    std::enable_if<is_parser<Parser>::value>();
    std::enable_if<is_parser<Delimiter>::value>();
    
    using namespace operators;
    
    auto delimitedMember = [=](State const &state) -> Result {
      return state
      >> delimiter
      >> member
      ;
    };
    
    return [=](State const &state) {
      return state
      >> member
      >> optional(repeat(delimitedMember));
    };
  }
  
  // Match content enclosed in brackets, optionally padded with whitespace
  // between the brackets.
  //
  // eg:
  //    sExp(match("<content>"))
  // would match:
  //    (<content>)
  template <typename Parser>
  auto sExp(Parser content) {
    std::enable_if<is_parser<Parser>::value>();
    
    using namespace operators;
    
    return [=](State const &state) -> Result {
      return state
      >> match("(")
      >> optionalWhitespace
      >> content
      >> optionalWhitespace
      >> match(")")
      ;
    };
  }
  
  // Match content enclosed in brackets, with an expression tag,
  // optionally padded between the brackets.
  //
  // eg:
  //    taggedSExp("foo", match("<content>"))
  // would match:
  //    (foo <content>)
  template <typename Parser>
  auto taggedSExp(char const *tag, Parser content) {
    std::enable_if<is_parser<Parser>::value>();
    
    using namespace operators;
    
    return [=](State const &state) -> Result {
      return state
      >> match("(")
      >> optionalWhitespace
      >> match(tag)
      >> whitespace
      >> content
      >> optionalWhitespace
      >> match(")")
      ;
    };
  }
  
  // A valid identifier
  template <typename Action>
  auto identifierString(Action const &out) {
    using namespace operators;
    
    auto identifierChar = printableChar && !parenChar;
    auto start = identifierChar && !digitChar;
    
    return [=](State const &state) {
      Arena::string result(state.allocator<char>());
      
      return state
      >> match(start, buildString(&result))
      >> optional(repeat(match(identifierChar, buildString(&result))))
      >> emit(&result, out)
      ;
    };
  }
  
  template <typename T = double, typename Action>
  auto real(Action const &out) {
    using namespace parse::operators;
    
    return [=](State const &state) -> Result {
      Arena::string digits(state.allocator<char>());
      auto addDigit = buildString(&digits);
      
      auto trailing = [=](State const &state) -> Result {
        return state
        >> match(".") >> emit(".", addDigit)
        >> repeat(match(digitChar, addDigit))
        ;
      };
      
      return state
      >> repeat(match(digitChar, addDigit))
      >> optional(trailing)
      >> inject([&](){
        auto value = atof(digits.c_str());
        out(static_cast<T>(value));
      });
    };
  }
  
  template <typename T = int64_t, typename Action>
  auto integer(Action const &out) {
    using namespace parse::operators;
    
    return [=](State const &state) -> Result {
      Arena::string digits(state.allocator<char>());
      
      return state
      >> repeat(match(digitChar, buildString(&digits)))
      >> inject([&](){
        auto value = atoll(digits.c_str());
        out(static_cast<T>(value));
      });
    };
  }
}

using namespace parse::operators;
