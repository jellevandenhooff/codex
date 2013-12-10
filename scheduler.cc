#include "scheduler.h"

#include <cassert>

#include <atomic>
#include <functional>
#include <map>

Scheduler::Scheduler() : current_thread_(kOriginalThread) {
  contexts_[kOriginalThread] = &original_context_;

  for (int i = 0; i < kMaxThreads; i++) {
    stacks_[i] = new uint8_t[kStackSize];
  }
}

Scheduler::~Scheduler() {
  for (int i = 0; i < kMaxThreads; i++) {
    delete[] stacks_[i];
  }
}

void Scheduler::SwitchTo(int new_thread) {
  if (new_thread == current_thread_) {
    return;
  }
  int thread = current_thread_;
  current_thread_ = new_thread;
  ctx::jump_fcontext(contexts_[thread], contexts_[current_thread_],
      reinterpret_cast<intptr_t>(this));
}

void Scheduler::AddThread(int thread, std::function<void()> task) {
  contexts_[thread] = ctx::make_fcontext(stacks_[thread] + kStackSize,
      kStackSize, &Scheduler::ThreadEntryPointWrapper);
  tasks_[thread] = task;
}

void Scheduler::ThreadEntryPointWrapper(intptr_t p) {
  reinterpret_cast<Scheduler*>(p)->ThreadEntryPoint();
}

void Scheduler::ThreadEntryPoint() {
  tasks_[current_thread_]();
  assert(0);
}

