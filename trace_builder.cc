#include "trace_builder.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#include "interceptor.h"

std::string TraceNode::CalculatePath() const {
  std::vector<int> path;

  auto current = this;
  while (current->parent_) {
    path.push_back(current->last_thread_);
    current = current->parent_.get();
  }

  std::reverse(path.begin(), path.end());

  std::stringstream ss;
  for (int thread : path) {
    ss << thread;
  }
  return ss.str();
}

TraceBuilder::TraceBuilder(Interceptor* interceptor, HHBHistory* history) :
    interceptor_(interceptor), history_(history) {
  interceptor_->StartNewRun(history_);
  root_ = current_ = std::shared_ptr<TraceNode>(new TraceNode());
  FillTraceNodeFromInterceptor(*current_);
}

void TraceBuilder::MoveTo(std::shared_ptr<TraceNode> node) {
  std::vector<int> path;

  auto base = node;
  while (base != current_ && base->parent()) {
    path.push_back(base->last_thread());
    base = base->parent();
  }
  std::reverse(path.begin(), path.end());

  if (base != current_) {
    interceptor_->StartNewRun(history_);
  }

  for (int thread : path) {
    interceptor_->AdvanceThread(thread);
  }

  current_ = node;
}

std::shared_ptr<TraceNode> TraceBuilder::Extend(int thread) {
  interceptor_->AdvanceThread(thread);

  if (!current_->next_[thread].expired()) {
    // FIXME: Not thread safe?
    current_ = current_->next_[thread].lock();
  } else {
    current_ = std::shared_ptr<TraceNode>(new TraceNode(current_, thread));
    current_->parent()->next_[thread] = current_;
    FillTraceNodeFromInterceptor(*current_);
  }

  return current_;
}

void TraceBuilder::FillTraceNodeFromInterceptor(TraceNode& node) {
  node.next_transitions_ = interceptor_->next_transitions();
  node.runnable_ = interceptor_->runnable();
}

