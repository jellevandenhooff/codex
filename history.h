#pragma once

#include <vector>

#include "threadmap.h"
#include "transition.h"

class History {
 public:
  virtual void AddTransition(int thread, const Transition& transition) {
    thread_at_.push_back(thread);
    transition_at_.push_back(transition);
    // FIXME: Guarantee that reading the previous value here is correct!
    previous_value_at_.push_back(transition.Read());
  }
  virtual void Reset() {
    thread_at_.clear();
    transition_at_.clear();
    previous_value_at_.clear();
  }

  inline const Transition& transition_at(int time) const {
    return transition_at_[time];
  }
  inline int thread_at(int time) const {
    return thread_at_[time];
  }
  inline int64_t previous_value_at(int time) const {
    return previous_value_at_[time];
  }
  inline int length() const {
    return thread_at_.size();
  }

  virtual void Dump() const {
    FILE* f = fopen("data.py", "w");
    fprintf(f, "data = [");
    for (int time = 0; time < length(); time++) {
      if (time > 0) {
        fprintf(f, ",\n");
      }

      int thread = thread_at(time);
      const Transition& transition = transition_at(time);

      if (transition.annotations()) {
        for (std::string annotation : *transition.annotations()) {
          fprintf(f, 
              "{'thread': %d, 'type': 'annotation', 'description': '%s'},\n", 
              thread, annotation.c_str());
        }
      }

      fprintf(f, "%s", 
          transition.Dump(thread, time, previous_value_at(time)).c_str());
    }
    fprintf(f, "]\n");
    fclose(f);
  }

 private:
  std::vector<int> thread_at_;
  std::vector<Transition> transition_at_;
  std::vector<int64_t> previous_value_at_;
};

