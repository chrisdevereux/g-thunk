#include <iostream>
#include <fstream>
#include <sstream>

#include "SerializeAST.hpp"
#include "AST.hpp"

// Ensure that serialized AST nodes unserialize as an equal node.
int main(int argc, char const **argv) {
  std::vector<char const *> args(argv + 1, argv + argc);
  size_t failCount = 0;
  
  for (auto filepath : args) {
    using namespace parse;
    using namespace ast;
    
    Arena arena;
    
    std::ifstream input(filepath);
    Module inputModule(&arena);
    
    if (!read(input, &arena, unserialize::module(receive(&inputModule)))) {
      std::cout << "FAILED: " << filepath << std::endl
      << "Invalid input AST" << std::endl;
      
      failCount++;
      continue;
    }
    
    std::stringstream serialized;
    serialized << inputModule;
    
    serialized.seekg(0, serialized.beg);
    Module unserializedModule(&arena);
    
    if (!read(serialized, &arena, unserialize::module(receive(&unserializedModule)))) {
      std::cout << "FAILED: " << filepath << std::endl
      << "Invalid serialized AST:" << std::endl
      << serialized.str();
      
      failCount++;
      continue;
    }
    
    if (unserializedModule != inputModule) {
      std::cout << "FAILED: " << filepath << std::endl
      << "Expected input AST to equal reparsed AST:" << std::endl
      << serialized.str();
      
      failCount++;
      continue;
    }
    
    std::cout << ".";
  }
  
  std::cout << std::endl << failCount << "/" << args.size() << " Failed" << std::endl;
  return failCount == 0 ? 0 : 1;
}
