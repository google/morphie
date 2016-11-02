// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

// Utilities for implementing simple mathematical operations on maps.
#ifndef LOGLE_UTIL_MAP_UTILS_H_
#define LOGLE_UTIL_MAP_UTILS_H_

#include <unordered_map>
#include <unordered_set>

namespace morphie {
namespace util {

// The type unordered_map<A, B> can be viewed as a partial function from values
// of type A to values of type B. The composition of f: A --> B with g: B --> C
// is the function h : A --> C that maps each element 'a' in the domain of 'f'
// to the element 'g(f(a))' provided 'f(a)' is in the domain of 'g'.  The C++
// functions below assume that the domain and the range of the function are over
// the same set.
//
// Returns the composition of the keys map with the values map. The result
// contains keys from the first map and values from the second map.
template <typename KeyType>
std::unordered_map<KeyType, KeyType> Compose(
    const std::unordered_map<KeyType, KeyType>& keys,
    const std::unordered_map<KeyType, KeyType>& values) {
  std::unordered_map<KeyType, KeyType> composition;
  for (const auto& key_pair : keys) {
    auto value_it = values.find(key_pair.second);
    if (value_it == values.end()) {
      continue;
    }
    composition.insert({key_pair.first, value_it->second});
  }
  return composition;
}

// The preimage of a function f : A --> B is a function from B to the powerset
// of A that maps each element 'b' to the set {a | f(a) = b}.
//
// Returns the preimage of the input function.
template <typename KeyType, typename ValType>
std::unordered_map<ValType, std::unordered_set<KeyType>> Preimage(
    const std::unordered_map<KeyType, ValType>& fn) {
  std::unordered_map<ValType, std::unordered_set<KeyType>> preimage;
  for (const auto& fn_pair : fn) {
    // If the insertion below fails, there is already an entry for this value in
    // the preimage map, so the key has to be inserted into the preimage set.
    auto insert_pair = preimage.insert({fn_pair.second, {fn_pair.first}});
    if (insert_pair.second == false) {
      insert_pair.first->second.insert(fn_pair.first);
    }
  }
  return preimage;
}

}  // namespace util
}  // namespace morphie

#endif  // LOGLE_UTIL_MAP_UTILS_H_
