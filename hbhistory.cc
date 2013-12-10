#include "hbhistory.h"

std::vector<int> HBHistory::FindFirstConflicts(int thread, const Transition& transition) {
  Object& object = objects_[(intptr_t)transition.address()];

  std::vector<int> first_conflicts;

  std::vector<int>& conflicts = 
      transition.can_write() ? object.accesses : object.writes;
  for (int time : conflicts) {
    if (!time_happens_before_thread(time, thread)) {
      first_conflicts.push_back(time);
    }
  }

  return first_conflicts;
}

void HBHistory::AddTransition(int thread, const Transition& transition) {
  History::AddTransition(thread, transition);

  Object& object = objects_[(intptr_t)transition.address()];

  int time = length() - 1;

  current_cv_for_[thread][thread] = time;

  if (transition.can_write()) {
    current_cv_for_[thread].Maximize(object.access_cv);
    object.access_cv.Maximize(current_cv_for_[thread]);
    object.write_cv.Maximize(current_cv_for_[thread]);
    object.accesses.push_back(time);
    object.writes.push_back(time);
  } else {
    current_cv_for_[thread].Maximize(object.write_cv);
    object.access_cv.Maximize(current_cv_for_[thread]);
    object.accesses.push_back(time);
  }

  cv_at_.push_back(current_cv_for_[thread]);

  previous_time_of_thread_at_.push_back(last_time_of_[thread]);
  last_time_of_[thread] = length() - 1;
}

void HBHistory::Reset() {
  History::Reset();

  objects_.Reset();
  cv_at_.clear();
  current_cv_for_.clear();

  for (int i = 0; i < kMaxThreads; i++) {
    current_cv_for_[i] = ClockVector();
  }

  previous_time_of_thread_at_.clear();
  for (int i = 0; i < kMaxThreads; i++) {
    last_time_of_[i] = -1;
  }
}

