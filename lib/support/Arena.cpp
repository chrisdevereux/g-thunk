#include "Arena.hpp"
#include <cstdlib>

Arena::~Arena() {
  for (auto x : mem) {
    free(x);
  }
}

// Add a new arena page capable of storing at least minSize
// and allocate all memory until the next call to this page
void Arena::pushPage(size_t minSize) {
  capacity = (minSize / pageSize + 1) * pageSize;
  assert(capacity >= minSize);
  
  offset = 0;
  
  mem.push_back((uint8_t *)malloc(capacity));
}
