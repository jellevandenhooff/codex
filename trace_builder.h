#pragma once

#include <functional>
#include <memory>

#include "threadset.h"
#include "threadmap.h"
#include "transition.h"

class TraceBuilder;

class TraceNode {
 public:
  inline std::shared_ptr<TraceNode> parent() const {
    return parent_;
  }
  inline int last_thread() const {
    assert(parent_);
    return last_thread_;
  }
  inline ThreadSet runnable() const {
    return runnable_;
  }
  inline ThreadMap<Transition> next_transitions() const {
    return next_transitions_;
  }
  inline bool is_leaf() const {
    return next_transitions_.size() == 0;
  }

  std::string CalculatePath() const;

 private:
  TraceNode() : parent_(nullptr) {}
  TraceNode(std::shared_ptr<TraceNode> parent, int last_thread) :
    parent_(parent), last_thread_(last_thread) {}

  std::shared_ptr<TraceNode> parent_; // null for root
  int last_thread_; // undefined for root

  ThreadSet runnable_;
  ThreadMap<Transition> next_transitions_;

  ThreadMap<std::weak_ptr<TraceNode>> next_; // useful cache to ensure pointer equality

  friend class TraceBuilder;
};

class Interceptor;
class HHBHistory;
class TraceBuilder {
 public:
  TraceBuilder(Interceptor* interceptor, HHBHistory* history);

  void MoveTo(std::shared_ptr<TraceNode> node);
  std::shared_ptr<TraceNode> Extend(int thread);

  inline std::shared_ptr<TraceNode> root() const {
    return root_;
  }
  inline std::shared_ptr<TraceNode> current() const {
    return current_;
  }

 private:
  Interceptor* interceptor_;
  HHBHistory* history_;

  std::shared_ptr<TraceNode> root_;
  std::shared_ptr<TraceNode> current_;

  void FillTraceNodeFromInterceptor(TraceNode& node);
};

