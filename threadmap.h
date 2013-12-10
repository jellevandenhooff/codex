#pragma once

#include "config.h"
#include "threadset.h"

#include <cassert>
#include <cstdint>

template<class T>
class ThreadMap {
 public:
  ThreadMap() : have() {
    for (int i = 0; i < kMaxThreads; i++) {
      data[i] = T();
    }
  }

  inline T& operator[](int thread) {
    have.insert(thread);
    return data[thread];
  }

  inline const T& operator[](int thread) const {
    assert(have.count(thread));
    return data[thread];
  }

  inline void erase(int thread) {
    have.erase(thread);
    data[thread] = T();
  }

  inline void clear() {
    have.clear();
    for (int i = 0; i < kMaxThreads; i++) {
      data[i] = T();
    }
  }

  inline bool count(int thread) const {
    return have.count(thread);
  }

  inline ThreadSet keys() const {
    return have;
  }

  inline int size() const {
    return have.size();
  }

 private:
  ThreadSet have;
  T data[kMaxThreads];
};

