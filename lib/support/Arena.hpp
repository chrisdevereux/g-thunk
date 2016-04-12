#pragma once

#include <cassert>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Fast pooling memory allocator.
//
// Allocates a large memory buffer and allocates objects by incrementing
// a pointer.
//
// Continues to allocate buffers once it fills, postponing deallocation until
// the arena object iself destucts.
//
// Deallocators are not called, so this should not be used for
// allocating RAII types.
//
// [todo] - checked whitelist of compatible types?
class Arena {
  std::vector<uint8_t *> mem;
  size_t pageSize;
  
  size_t capacity = 0;
  size_t offset = 0;
  
public:
  template <typename T>
  struct Allocator;
  
  template <typename T>
  using vector = std::vector<T, Allocator<T>>;
  
  template <typename Key, typename Val>
  using unordered_map = std::unordered_map<Key, Val, std::hash<Key>, std::equal_to<Key>, Arena::Allocator<std::pair<Key const, Val>>>;
  
  template <typename Val>
  using unordered_set = std::unordered_set<Val, std::hash<Val>, std::equal_to<Val>, Arena::Allocator<Val>>;
  
  using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
  
  explicit Arena(size_t pageSize_ = 4096)
  : pageSize(pageSize_)
  {}
  
  ~Arena();
  
  // Return an stl-compatible allocator for elements of type T
  template <typename T>
  Allocator<T> allocator() {
    return Allocator<T>(this);
  }
  
  // Allocate and construct a single arena object of type T.
  template <typename T, typename ...Params>
  inline T *create(Params ...params) {
    return new(allocN<T>(1)) T(params...);
  }
  
  // Allocate contiguous buffer for n elements of T
  // Does not construct the objects.
  template <typename T>
  inline T *allocN(size_t count) {
    size_t allocSize = count * sizeof(T);
    
    if (alignedSlot<T>() + allocSize >= capacity) {
      pushPage(allocSize);
    }
    
    T *ptr = (T *)(mem.back() + alignedSlot<T>());
    offset = alignedSlot<T>() + allocSize;
    
    return ptr;
  }
  
private:
  void pushPage(size_t minSize);
  
  // Return the offset of the first slot in the current page
  //
  template <typename T>
  inline size_t alignedSlot() const {
    size_t alignMiss = offset % alignof(T);
    size_t slot = alignMiss ? (offset + alignof(T) - alignMiss) : offset;
    
    assert(slot % alignof(T) == 0);
    return slot;
  }
};

// STL-compatible allocator that allocates in an arena.
template <typename T>
struct Arena::Allocator {
  typedef T value_type;
  Arena *arena;
  
  explicit Allocator(Arena *arena_)
  : arena(arena_)
  {}
  
  template <typename T_>
  Allocator(Allocator<T_> const &rhs)
  : arena(rhs.arena)
  {}
  
  T *allocate(std::size_t n) {
    return arena->allocN<T>(n);
  }
  
  void deallocate(T *p, size_t n) {
    // No-op
  }
};

template <typename T>
bool operator ==(Arena::Allocator<T> const &lhs, Arena::Allocator<T> const &rhs) {
  return lhs.arena == rhs.arena;
}

template <typename T>
bool operator !=(Arena::Allocator<T> const &lhs, Arena::Allocator<T> const &rhs) {
  return lhs.arena != rhs.arena;
}
