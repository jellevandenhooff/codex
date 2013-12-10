#pragma once

#include "program_interface.h"

#include <atomic>

#include "config.h"
#include "clockvector.h"

// Interceptor uses kMaxThreads as ID for current thread for original thread,
// which is safe as no algoritm will ever handle a transition by the original
// thread.  However, client code needs to be aware of this possible ID as the
// original thread can still use ThreadID() for e.g. thread-local storage.
static const int kMaxThreadId = kMaxThreads + 1; 

class Mutex {
 public:
  void Reset() {
    held = false;
  }

  void Acquire() {
    bool old = false;
    RequireResult(old);
    while (!held.compare_exchange_weak(old, true)) {}
  }

  bool TryAcquire() {
    bool old = false;
    return held.compare_exchange_weak(old, true);
  }

  void Release() {
    held = false;
  }

 private:
  std::atomic<bool> held;
};

class RecursiveMutex {
 public:
  void Reset() {
    held = false;
    for (int i = 0; i < kMaxThreadId; i++) {
      count[i] = 0;
    }
  }

  void Acquire() {
    if (count[ThreadId()]++ > 0) {
      return;
    }

    bool old = false;
    RequireResult(old);
    while (!held.compare_exchange_weak(old, true)) {}
  }

  bool TryAcquire() {
    bool old = false;
    if (count[ThreadId()] > 0 ||
        held.compare_exchange_weak(old, true)) {
      count[ThreadId()]++;
      return true;
    } else {
      return false;
    }
  }

  void Release() {
    if (--count[ThreadId()] > 0) {
      return;
    }

    held = false;
  }

 private:
  std::atomic<bool> held;
  int count[kMaxThreadId];
};

template<class T>
class ThreadLocalStorage {
 public:
  void Reset() {
    for (int i = 0; i < kMaxThreadId; i++) {
      data[i] = T();
    }
  }

  T& Get() {
    return data[ThreadId()];
  }

 private:
  T data[kMaxThreadId];
};

