#include "interceptor.h"

#include <set>

#include "hhbhistory.h"
#include "statistics.h"

static int64_t& total_runs = RegisterStatistic<int64_t>("runs");
static int64_t& total_transitions = RegisterStatistic<int64_t>("transitions");
static int64_t& total_found = RegisterStatistic<int64_t>("found");
static int64_t& total_distinct = RegisterStatistic<int64_t>("distinct");
static int& first_found = RegisterStatistic<int>("first_found", -1);
static std::set<Hash> seen_hashes;

void Interceptor::StartNewRun(HHBHistory* history) {
  while (!runnable_.empty()) {
    AdvanceThread(*runnable_.begin());
  }

  assert(alive_threads_.empty());
  num_created_threads_ = 0;
  has_found_bug_ = false;

  history_ = history;
  // TODO: Clarify history ownership and Reset duty.
  if (history_ != nullptr) {
    history_->Reset();
  }

  total_runs++;

  setup_run_();
  SwitchToNext();
  ComputeRunnable();
}

int Interceptor::StartThread(const std::function<void()>& task) {
  assert(num_created_threads_ < kMaxThreads);

  int thread = num_created_threads_++;

  scheduler_.AddThread(thread, [=]() {
    task();
    alive_threads_.erase(thread);
    SwitchToNext();
  });

  alive_threads_.insert(thread);

  return thread;
}

void Interceptor::AdvanceThread(int thread) {
  assert(alive_threads_.count(thread));
  assert(next_transitions_.count(thread));

  // DANGER!! AddTransition assumes that it is called right before the
  // transition is executed and so SHOULD NOT BE MOVED.
  if (history_ != nullptr) {
    history_->AddTransition(thread, next_transitions_[thread]);
  }

  // The thread will call ReachedTransition before switching back to this
  // thread, so we must delete the old transition before switching to the
  // thread.
  next_transitions_.erase(thread);

  total_transitions++;

  scheduler_.SwitchTo(thread);
  ComputeRunnable();
}

void Interceptor::FinishRun() {
  finish_run_();

  if (has_found_bug_) {
    if (total_found++ == 0) {
      history_->Dump();
      first_found = total_runs;
    }
  }
  if (seen_hashes.insert(history_->CombineCurrentHashes()).second) {
    total_distinct++;
  }
}

void Interceptor::ComputeRunnable() {
  runnable_.clear();
  for (int thread : next_transitions_.keys()) {
    if (next_transitions_[thread].DetermineRunnable()) {
      runnable_.insert(thread);
    }
  }

  if (alive_threads_.empty()) {
    FinishRun();
  } else if (runnable_.empty()) {
    // TODO: Handle found deadlock gracefully. A deadlock is a difficult case
    // as the tested program can not be reset to a normal state, so we have to
    // stop. However, it would be useful to dump the violating trace.
    assert(0);
    exit(0);
  }
}

void Interceptor::SwitchToNext() {
  ThreadSet next_unknown = alive_threads_ - next_transitions_.keys();

  if (!next_unknown.empty()) {
    scheduler_.SwitchTo(*next_unknown.begin());
  } else {
    scheduler_.SwitchTo(Scheduler::kOriginalThread);
  }
}

void Interceptor::ReachedTransition(const Transition& transition) {
  int thread = scheduler_.current_thread();
  assert(!next_transitions_.count(thread));

  next_transitions_[thread] = transition;
  SwitchToNext();
}

