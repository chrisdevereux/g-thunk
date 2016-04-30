#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "ParseUtil.hpp"

// Ensure that serialized values unserialize as an equal value.
template <typename Value, typename Compare>
int serializationTest(int argc, char const **argv, parse::Grammar(*parser)(parse::GenericAction<Value>), Compare compare) {
  using std::unique_ptr;
  
  std::vector<char const *> args(argv + 1, argv + argc);
  size_t passCount = 0;
  
  bool logging = atoi(getenv("TEMPO_TEST_LOGGING") ?: "0") > 0;
  
  for (auto filepath : args) {
    using namespace parse;
    
    Arena arena;
    
    std::ifstream input(filepath);
    std::vector<std::string> errors;
    
    unique_ptr<Value> inputValue;
    unique_ptr<Value> unserializedValue;
    
    if (!read(input, &arena, parser(receivePointerValue(&inputValue)), &errors)) {
      std::cout << std::endl << "FAILED: " << filepath << std::endl
      << "Invalid input representation" << std::endl;
      
      for (auto err : errors) {
        std::cout << err << std::endl;
      }
      
      continue;
    }
    
    std::stringstream serialized;
    serialized << *inputValue;
    
    serialized.seekg(0, serialized.beg);
    
    if (!read(serialized, &arena, parser(receivePointerValue(&unserializedValue)), &errors)) {
      std::cout << std::endl << "FAILED: " << filepath << std::endl
      << "Invalid serialized representation:" << std::endl
      << serialized.str();
      
      for (auto err : errors) {
        std::cout << err << std::endl;
      }
      
      continue;
    }
    
    if (!compare(*unserializedValue, *inputValue)) {
      std::cout << std::endl << "FAILED: " << filepath << std::endl
      << "Expected input representation to equal reparsed representation:" << std::endl
      << serialized.str();
      
      continue;
    }
    
    passCount++;
    
    if (logging) {
      std::cout << "." << std::endl << serialized.str();
      
    } else {
      std::cout << ".";
    }
  }
  
  std::cout << std::endl
  << "Tests Completed" << std::endl
  << passCount << "/" << args.size() << " Passed" << std::endl;
  
  return (passCount == args.size()) ? 0 : 1;
}

template <typename Value>
int serializationTest(int argc, char const **argv, parse::Grammar(*parser)(parse::GenericAction<Value const &>)) {
  return serializationTest(argc, argv, parser, std::equal_to<Value>());
}
