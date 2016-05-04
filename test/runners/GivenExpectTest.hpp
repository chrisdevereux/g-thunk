#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <memory>

#include "ParseUtil.hpp"

template <typename T>
using Parser = parse::Grammar(*)(parse::GenericAction<T>);

template <typename GivenValue, typename ExpectedValue, typename Convert>
int givenExpectTest(int argc, char const *const *argv, Parser<GivenValue> given, Parser<ExpectedValue> expect, Convert convert) {
  using std::unique_ptr;
  using namespace parse;
  
  std::vector<char const *> args(argv + 1, argv + argc);
  size_t passCount = 0;
  
  for (auto filepath : args) {
    Arena arena;
    unique_ptr<GivenValue> givenVal;
    unique_ptr<ExpectedValue> expectedVal;
    
    auto parseTest = [&](State const &state) -> Result {
      return state
      >> optionalWhitespace
      >> requiredMatch("@given:") >> require("given clause value", optionalWhitespace >> given(receivePointerValue(&givenVal))) >> optionalWhitespace
      >> requiredMatch("@expect:") >> require("expect clause value", optionalWhitespace >> expect(receivePointerValue(&expectedVal))) >> optionalWhitespace
      >> optionalWhitespace
      >> require("EOF", eof())
      ;
    };
    
    std::ifstream input(filepath);
    std::vector<std::string> errors;
    
    if (!read(input, &arena, parseTest, &errors)) {
      std::cout << "FAILED: " << filepath << std::endl
      << "Invalid input representation" << std::endl;
      
      for (auto x : errors) std::cout << x << std::endl;
      
      continue;
    }
    
    auto actualVal = convert(*givenVal);
    if (actualVal != *expectedVal) {
      std::stringstream ss;
      ss << actualVal;
      
      std::cout << "FAILED: " << filepath << std::endl
      << "Expected:" << std::endl
      << *givenVal << std::endl
      << "to equal:" << std::endl
      << *expectedVal << std::endl
      << "but got:" << std::endl
      << ss.str() << std::endl
      << "instead" << std::endl << std::endl
      ;
      
      continue;
    }
    
    std::cout << ".";
    ++passCount;
  }
  
  std::cout << std::endl
  << "Tests Completed" << std::endl
  << passCount << "/" << args.size() << " Passed" << std::endl;
  
  return (passCount == args.size()) ? 0 : 1;
}

template <typename Value>
int givenExpectTest(int argc, char const *const *argv, Parser<Value> given, Parser<Value> expect) {
  return givenExpectTest(argc, argv, given, expect, [](Value const &v) {
    return v;
  });
}
