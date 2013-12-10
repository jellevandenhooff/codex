#include "helper.h"
#include "linearizability.h"

#include <functional>
#include <string>
#include <sstream>
#include <vector>

Linearizability::Linearizability(int num_threads) {
  threads.resize(num_threads);
}

void Linearizability::RegisterModel(std::function<void()> setup, std::function<void()> cleanup) {
  setup_model = setup;
  cleanup_model = cleanup;
}

void Linearizability::RegisterImplementation(std::function<void()> setup, std::function<void()> cleanup) {
  setup_impl = setup;
  cleanup_impl = cleanup;
}

void Linearizability::AddStep(int thread, std::function<int()> function, std::string name) {
  threads[thread].push_back(std::make_pair(function, name));
}

void Linearizability::Setup() {
  setup_impl();

  order.clear();
}

void Linearizability::Finish() {
  cleanup_impl();

  linearization.clear();

  if (!Search()) {
    /*
    for (int i = 0; i < order.size(); i++) {
      if (order[i].finished) {
        Output("%d: %s -> %d\n", order[i].thread, threads[order[i].thread][order[i].function].second.c_str(), order[i].result);
      } else {
        Output("%d: %s start\n", order[i].thread, threads[order[i].thread][order[i].function].second.c_str());
      }
    }
    Output("\n");
    */
    Found();
    //exit(0);
  }
}

void Linearizability::ThreadBody(int thread) {
  for (int i = 0; i < threads[thread].size(); i++) {
    int start = order.size();

    order.push_back(Ordering());
    order[start].thread = thread;
    order[start].actual_thread = ThreadId();
    order[start].function = i;
    order[start].start_cv = GetClockVector(thread);
    order[start].executed = false;
    {
      std::stringstream ss;
      ss << "Starting " << threads[thread][i].second.c_str();
      Annotate(ss.str());
    }
    int ret = threads[thread][i].first();
    {
      std::stringstream ss;
      ss << "-> " << ret;
      Annotate(ss.str());
    }
    order[start].end_cv = GetClockVector(thread);
    order[start].result = ret;
  }
}

bool Linearizability::Verify() {
  setup_model();

  bool success = true;
  for (int idx : linearization) {
    Ordering& o = order[idx];
    if (threads[o.thread][o.function].first() != o.result) {
      success = false;
      break;
    }
  }

  cleanup_model();

  return success;
}

bool Linearizability::Search() {
  if (!Verify()) {
    return false;
  }

  bool done = true;
  for (int i = 0; i < order.size(); i++) {
    if (!order[i].executed) {
      done = false;
      break;
    }
  }
  if (done) {
    return true;
  }

  for (int i = 0; i < order.size(); i++) {
    if (order[i].executed) {
      continue;
    }

    bool can = true;
    for (int j = 0; j < order.size(); j++) {
      if (i == j) {
        continue;
      }

      if (order[j].executed) {
        continue;
      }

      if (order[i].thread == order[j].thread) {
        if (j < i) {
          can = false;
          break;
        } else {
          continue;
        }
      }

      bool i_after_j = order[i].end_cv[order[j].actual_thread] >= order[j].start_cv[order[j].actual_thread];
      bool j_after_i = order[j].end_cv[order[i].actual_thread] >= order[i].start_cv[order[i].actual_thread];

      //assert(i_after_j || j_after_i);

      if (i_after_j && !j_after_i) {
        can = false;
        break;
      }
    }

    if (can) {
      linearization.push_back(i);
      order[i].executed = true;
      bool success = Search();
      order[i].executed = false;
      linearization.pop_back();
      if (success) {
        return true;
      }
    }
  }

  return false;
}
