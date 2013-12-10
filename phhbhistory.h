#pragma once

#include <vector>

#include "hhbhistory.h"

class PHHBHistory : public HHBHistory {
 public:
  virtual void AddTransition(int thread, const Transition& transition) {
    first_conflicts_at_.push_back(FindFirstConflicts(thread, transition));
    previous_time_of_thread_at_.push_back(current_cv_for(thread)[thread]);
    HHBHistory::AddTransition(thread, transition);
  }

  virtual void Reset() {
    HHBHistory::Reset();
    first_conflicts_at_.clear();
    previous_time_of_thread_at_.clear();
  }

  inline const std::vector<int>& first_conflicts_at(int time) const {
    return first_conflicts_at_[time];
  }

  inline int previous_time_of_thread_at(int time) const {
    return previous_time_of_thread_at_[time];
  }

 private:
  std::vector<std::vector<int>> first_conflicts_at_;
  std::vector<int> previous_time_of_thread_at_;
};

