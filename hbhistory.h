#pragma once

#include <vector>

#include "clockvector.h"
#include "hashtable.h"
#include "history.h"
#include "threadmap.h"
#include "transition.h"

struct Object {
  std::vector<int> accesses;
  std::vector<int> writes;
  ClockVector access_cv, write_cv;

  // Objects are stored in a Hashtable that uses Reset to clear out objects.
  void Reset() {
    accesses.clear();
    writes.clear();
    access_cv.Reset();
    write_cv.Reset();
  }
};

class HBHistory : public History {
 public:
  virtual void AddTransition(int thread, const Transition& transition);
  virtual void Reset();
  std::vector<int> FindFirstConflicts(int thread, const Transition& transition);

  inline bool time_happens_before_time(int a, int b) const {
    int thread = thread_at(a);
    return cv_at_[b][thread] >= cv_at_[a][thread];
  }
  inline bool time_happens_before_thread(int time, int thread) const {
    int other_thread = thread_at(time);
    return current_cv_for_[thread][other_thread] >= cv_at_[time][other_thread];
  }
  inline const ClockVector& cv_at(int time) const {
    return cv_at_[time];
  }
  inline ClockVector current_cv_for(int thread) const {
    return current_cv_for_[thread];
  }
  inline int64_t previous_time_of_thread_at(int time) const {
    return previous_time_of_thread_at_[time];
  }

  bool IsSplit(int a, int b) {
    int thread = thread_at(b);
    for (int other_thread = 0; other_thread < kMaxThreads; other_thread++) {
      if (thread == other_thread) {
        continue;
      }
      int seen_them = cv_at(b)[other_thread];
      if (seen_them != -1) {
        int seen_us = cv_at(seen_them)[thread];
        if (seen_us >= a) {
          return true;
        }
      }
    }
    return false;
  }

 private:
  HashTable<Object> objects_;
  std::vector<ClockVector> cv_at_;
  ThreadMap<ClockVector> current_cv_for_;
  std::vector<int> previous_time_of_thread_at_;
  ThreadMap<int> last_time_of_;
};

