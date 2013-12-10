#pragma once

#include <functional>

#include "scheduler.h"
#include "threadset.h"
#include "threadmap.h"
#include "transition.h"

class HHBHistory;

class Interceptor {
 public:
  Interceptor(const std::function<void()>& setup_run, 
      const std::function<void()>& finish_run) :
    setup_run_(setup_run), finish_run_(finish_run) {}

  int StartThread(const std::function<void()>& task);
  void ReachedTransition(const Transition& transition);

  // TODO: FoundBug can perhaps move into an annotation.
  inline void FoundBug() {
    has_found_bug_ = true;
  }

  void StartNewRun(HHBHistory* history);
  void AdvanceThread(int thread);  
  // TODO: AdvanceThread switches back to the originating thread after every
  // transition, which is not necessary if the originating thread can supply a
  // series of threads to run in advance, or if the transitions to run can be
  // determined in a callback.

  inline int current_thread() const {
    return scheduler_.current_thread();
  }

  inline ThreadSet runnable() const {
    return runnable_;
  }

  inline const ThreadMap<Transition>& next_transitions() const {
    return next_transitions_;
  }

  inline const HHBHistory* history() const {
    return history_;
  }

  inline bool has_found_bug() const {
    return has_found_bug_;
  }

  inline bool finished() const {
    return alive_threads_.empty();
  }

 private:
  void SwitchToNext();
  // FIXME: ComputeRunnable needs a better name to reflect that
  // it also checks for run end and deadlock.
  void ComputeRunnable();
  void FinishRun();

  std::function<void()> setup_run_, finish_run_;

  Scheduler scheduler_;
  ThreadSet alive_threads_, runnable_;
  ThreadMap<Transition> next_transitions_;

  bool has_found_bug_;

  // TODO: Consider if history really has a place in interceptor, and if so,
  // what subclass.
  HHBHistory* history_;

  int num_created_threads_;
};

