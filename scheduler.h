#pragma once

#include <atomic>
#include <functional>
#include <map>

#include <boost/context/all.hpp>

#include "config.h"

namespace ctx = boost::context;

class Scheduler {
 public:
  static const int kOriginalThread = kMaxThreads;
  static const size_t kStackSize = 1024 * 8;

  Scheduler();
  ~Scheduler();

  void SwitchTo(int new_thread);
  void AddThread(int thread, std::function<void()> task);

  inline int current_thread() const {
    return current_thread_;
  }

 private:
  std::function<void()> tasks_[kMaxThreads];

  ctx::fcontext_t original_context_;
  uint8_t* stacks_[kMaxThreads];
  ctx::fcontext_t* contexts_[kMaxThreads + 1];

  std::atomic<int> current_thread_;

  void ThreadEntryPoint();
  static void ThreadEntryPointWrapper(intptr_t p);
};

