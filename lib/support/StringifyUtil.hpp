#pragma once

#include "Arena.hpp"
#include "ParseUtil.hpp"

#include <stack>

// Utility class for pretty-printing S-expressions
class Stringifier {
public:
  Stringifier(std::ostream &str_)
  : str(str_)
  {
    indexStack.push(0);
  }
  
  template <typename T>
  void atom(T value) {
    printSep();
    
    str << value;
  }
  
  template <typename T, typename Visitor, typename Fn>
  void compound(T value, Visitor visitor, Fn visitFn) {
    (visitor->*visitFn)(&normalizedReference(value));
  }
  
  template <typename T, typename Visitor>
  void compound(T value, Visitor visitor) {
    normalizedReference(value).visit(visitor);
  }
  
  template <typename Coll>
  void each(Coll const &coll) {
    for (auto x : coll) {
      atom(normalizedReference(x));
    }
  }
  
  template <typename Coll>
  void each(Coll const &coll, char const *tag) {
    for (size_t i = 0; i < coll.size(); ++i) {
      atom(tag);
    }
  }
  
  template <typename Coll, typename Visitor, typename Fn>
  void each(Coll const &coll, Visitor visitor, Fn visitFn) {
    for (auto x : coll) {
      compound(x, visitor, visitFn);
    }
  }
  
  template <typename Coll, typename Visitor>
  void each(Coll const &coll, Visitor visitor) {
    for (auto x : coll) {
      compound(x, visitor);
    }
  }
  
  void begin(char const *tag) {
    printSep();
    
    ++indentation;
    indexStack.push(1);
    
    str << "(" << tag;
  }
  
  void begin() {
    printSep();
    
    ++indentation;
    indexStack.push(0);
    
    str << "(";
  }
  
  void end() {
    --indentation;
    indexStack.pop();
    
    str << ")";
  }
  
private:
  std::ostream &str;
  
  size_t indentation = 0;
  std::stack<size_t> indexStack;
  
  template <typename T>
  static T &normalizedReference(T &x) {
    return x;
  }
  
  template <typename T>
  static T &normalizedReference(T *x) {
    return *x;
  }
  
  void printSep() {
    if (indexStack.top() >= 1) {
      str << "\n";
      
      for (size_t i = 0; i < indentation; ++i) {
        str << "\t";
      }
    }
    
    ++indexStack.top();
  }
};
