#pragma once

#include <functional>
#include <algorithm>

template <typename T>
bool equalData(T const *lhs, T const *rhs) {
  return *lhs == *rhs;
}

template <typename L, typename R, typename Compare>
bool equalCollections(L const &lhs, R const &rhs, Compare const &compare) {
  return lhs.size() == rhs.size()
  && std::equal(lhs.begin(), lhs.end(), rhs.begin(), compare)
  ;
}
