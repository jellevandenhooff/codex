#pragma once

#include "config.h"

#include <cassert>
#include <algorithm>

class ClockVector {
 public:
  ClockVector(int value=-1) { Reset(value); }

  inline void Reset(int value=-1) {
    std::fill(times_, times_ + kMaxThreads, value);
  }

  inline void Maximize(const ClockVector& other) {
    for (int i = 0; i < kMaxThreads; i++)
      times_[i] = std::max(times_[i], other.times_[i]);
  }

  inline void Minimize(const ClockVector& other) {
    for (int i = 0; i < kMaxThreads; i++)
      times_[i] = std::min(times_[i], other.times_[i]);
  }

  inline int& operator[](int thread) {
    assert(0 <= thread && thread < kMaxThreads);
    return times_[thread];
  }

  inline int operator[](int thread) const {
    assert(0 <= thread && thread < kMaxThreads);
    return times_[thread];
  }

  inline bool happens_after_any(const ClockVector& other) const {
    for (int i = 0; i < kMaxThreads; i++) {
      if (times_[i] >= other.times_[i]) {
        return true;
      }
    }
    return false;
  }

  inline bool has_any_besides(int thread) const {
    for (int i = 0; i < kMaxThreads; i++) {
      if (i != thread && times_[i] != -1) {
        return true;
      }
    }
    return false;
  }

 private:
  int times_[kMaxThreads];
};

