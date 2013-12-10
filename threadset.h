#pragma once

#include "config.h"

#include <cassert>
#include <cstdint>

class ThreadSetIterator {
  friend class ThreadSet;

 public:
  inline ThreadSetIterator(uint64_t bitset) : bitset(bitset) {}

  inline ThreadSetIterator& operator++() {
    bitset &= bitset - 1;
    return *this;
  }

  inline int operator*() const {
    assert(bitset != 0);
    return __builtin_ctzl(bitset);
  }

  inline bool operator==(const ThreadSetIterator& o) const {
    return bitset == o.bitset;
  }

  inline bool operator!=(const ThreadSetIterator& o) const {
    return bitset != o.bitset;
  }

 public:
  uint64_t bitset;
};

class ThreadSet {
 public:
  inline ThreadSet() : bitset(0) {}
  inline void insert(int value) {
    assert(0 <= value && value < 64);
    bitset |= 1ULL << value;
  }

  inline static ThreadSet Singleton(int value) {
    assert(0 <= value && value < 64);
    return ThreadSet(1ULL << value);
  }

  inline void erase(int value) {
    assert(0 <= value && value < 64);
    bitset &= ~(1ULL << value);
  }

  inline bool empty() const {
    return bitset == 0;
  }

  inline void clear() {
    bitset = 0;
  }

  inline bool count(int value) const {
    assert(0 <= value && value < 64);
    return bitset & (1ULL << value);
  } 

  inline void add(const ThreadSetIterator& begin, const ThreadSetIterator& end) {
    bitset |= begin.bitset & ~end.bitset;
  }

  inline ThreadSetIterator begin() const {
    return ThreadSetIterator(bitset);
  }

  inline ThreadSetIterator upper_bound(int value) const {
    assert(0 <= value && value < 64);
    if (value == 63) {
      return 0;
    } else {
      return ThreadSetIterator(bitset & ~((1ULL << (value + 1)) - 1));
    }
  }

  inline int size() const {
    return __builtin_popcountl(bitset);
  }

  inline ThreadSetIterator end() const {
    return 0;
  }

  inline ThreadSet operator-(const ThreadSet& o) const {
    return ThreadSet(bitset & ~o.bitset);
  }

  inline ThreadSet operator&(const ThreadSet& o) const {
    return ThreadSet(bitset & o.bitset);
  }

  inline ThreadSet operator|(const ThreadSet& o) const {
    return ThreadSet(bitset | o.bitset);
  }

  inline bool operator==(const ThreadSet& o) const {
    return bitset == o.bitset;
  }

  inline bool operator!=(const ThreadSet& o) const {
    return bitset != o.bitset;
  }

  uint64_t bitset;
 private:
  inline ThreadSet(uint64_t bitset) : bitset(bitset) {}
};

