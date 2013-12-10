#include "pinner.h"

#include <vector>

#include <cstdio>
#include <climits>

#include "interceptor.h"
#include "phhbhistory.h"
#include "statistics.h"
#include "transition.h"

// NOTE: pinner is broken

extern Interceptor* interceptor;

static std::vector<PinnerState*> state_cache;

PinnerState* GetUnusedState() {
  if (state_cache.empty()) {
    return new PinnerState();
  } else {
    PinnerState* state = state_cache.back();
    state_cache.pop_back();
    return state;
  }
}

void ReturnUnusedState(PinnerState* state) {
  state_cache.push_back(state);
}

void PrepareStateForNewRun(PinnerState* state) {
  state->first_seen.clear();
  state->last_considered.clear();
  state->fixed.clear();
  state->is_a_pin.clear();
  state->last_pin.clear();
  state->cost = 0;
  state->thread_cost.clear();
}

static void Push(PinnerState* state, int first_seen, int last_considered, bool fixed,
    bool is_a_pin) {
  state->first_seen.push_back(first_seen);
  state->last_considered.push_back(last_considered);
  state->fixed.push_back(fixed);
  state->is_a_pin.push_back(is_a_pin);

  if (fixed) {
    int time = state->history.length() - 1;
    int thread = state->history.thread_at(time);

    if (!state->last_pin.count(thread)) {
      state->cost++;
      state->thread_cost[thread]++;
      state->last_pin[thread] = time;
    } else if (state->history.IsSplit(state->last_pin[thread], time)) {
      state->cost++;
      state->thread_cost[thread]++;
      state->last_pin[thread] = time;
    }
  }
}

int GetFirstRunnableThreadByParentFirstSeen(PinnerState* state) {
  int best_thread = -1;
  int best_parent_first_seen = INT_MAX;
  for (int thread : interceptor->runnable()) {
    int parent = state->history.current_cv_for(thread)[thread];
    int parent_first_seen = parent == -1 ? 0 : state->first_seen[parent];
    if (parent_first_seen < best_parent_first_seen) {
      best_thread = thread;
      best_parent_first_seen = parent_first_seen;
    }
  }
  return best_thread;
}

void CreateInitialState(PinnerState* state) {
  state->depth = 0;
  PrepareStateForNewRun(state);
  interceptor->StartNewRun(&state->history);

  int thread = *interceptor->runnable().begin();
  while (!interceptor->finished()) {
    //int thread = *interceptor->runnable().begin();
    if (!interceptor->runnable().count(thread)) {
      thread = *interceptor->runnable().begin();
    }
    interceptor->AdvanceThread(thread);
    Push(state, state->depth, -1, false, false);
  }
}

void Pin(PinnerState* state, const Choice& choice, const PinnerState* old) {
  int thread = old->history.thread_at(choice.time);

  state->depth = old->depth + 1;
  PrepareStateForNewRun(state);
  interceptor->StartNewRun(&state->history);

  ThreadMap<int> special_last_considered;

  for (int time = 0; time < old->history.length(); time++) {
    int thread = old->history.thread_at(time);

    if (!old->history.cv_at(time).happens_after_any(choice.c)) {
      interceptor->AdvanceThread(thread);

      if (time < choice.time) {
        Push(state, old->first_seen[time], old->depth,
            old->fixed[time], old->is_a_pin[time]);
      } else {
        Push(state, old->first_seen[time], old->last_considered[time],
            old->fixed[time], old->is_a_pin[time]);
      }
    } else {
      if (!special_last_considered.count(thread)) {
        special_last_considered[thread] = old->last_considered[time];
      }
    }
  }

  int pin_point = state->history.length();
  interceptor->AdvanceThread(thread);
  assert(special_last_considered.count(thread));
  Push(state, state->depth, special_last_considered[thread], true, true);
  special_last_considered.erase(thread);
  for (int i = 0; i < pin_point; i++) {
    if (state->history.time_happens_before_time(i, pin_point)) {
      state->fixed[i] = true;
    }
  }

  while (!interceptor->finished()) {
    //thread = GetFirstRunnableThreadByParentFirstSeen(state);
    if (!interceptor->runnable().count(thread)) {
      thread = *interceptor->runnable().begin();
    }
    interceptor->AdvanceThread(thread);

    if (special_last_considered.count(thread)) {
      Push(state, state->depth, special_last_considered[thread], false, false);
      special_last_considered.erase(thread);
    } else {
      Push(state, state->depth, -1, false, false);
    }
  }
}

int64_t& pinner_states = RegisterStatistic<int64_t>("pinner-states");

void ConsiderPin(PinnerState* state, 
    std::vector<int>::const_reverse_iterator index, 
    std::vector<int>::const_reverse_iterator end,
    const ClockVector& b,
    bool b_nonempty,
    ClockVector& c,
    bool c_nonempty,
    int64_t value,
    int pin_time,
    int max_cost,
    std::vector<ClockVector>& cs) {

  // Ensure that the last transition, which is the first one we pick, has not
  // been considered before.

  int index_first_seen = 0;
  if (index != end) {
    index_first_seen = state->first_seen[*index];
  }
  // first_seen is non-decreasing, so if this check fails now, it will also
  // fail for all future choices.
  if (!b_nonempty && index_first_seen <= state->last_considered[pin_time]) {
    return;
  }

  // Ensure that our transition is runnable after the last transition we pick.
  bool can_put_in_b = b_nonempty || 
    state->history.transition_at(pin_time).DetermineRunnable(value);

  // Cost estimation logic
  if (index != end && state->cost == max_cost) {
    int pin_thread = state->history.thread_at(pin_time);
    if (state->last_pin.count(pin_thread)) {
      int previous_pin = state->last_pin[pin_thread];
      if (state->history.cv_at(*index)[pin_thread] >= previous_pin) {
        can_put_in_b = false;
      }
    } else {
      can_put_in_b = false;
    }
  }

  if (index != end) {
    auto next_index = index;
    next_index++;

    if (can_put_in_b) {
      ClockVector new_b = b;
      new_b.Maximize(state->history.cv_at(*index));
      ConsiderPin(state, next_index, end, new_b, true, c, c_nonempty,
          state->history.previous_value_at(*index), pin_time, max_cost,
          cs);
    } 
    
    int index_thread = state->history.thread_at(*index);
    bool can_put_in_c = b[index_thread] < *index && !state->fixed[*index];
    if (can_put_in_c) {
      int old_value = c[index_thread];
      c[index_thread] = *index;
      ConsiderPin(state, next_index, end, b, b_nonempty, c, true,
          state->history.previous_value_at(*index), pin_time, max_cost,
          cs);
      c[index_thread] = old_value;
    }
  } else if (can_put_in_b && c_nonempty) {
    cs.push_back(c);
  }
}


std::vector<Choice> GenerateChoices(PinnerState* state, int max_cost) {
  std::vector<Choice> choices;
  for (int time = 0; time < state->history.length(); time++) {
    int thread = state->history.thread_at(time);

    if (state->fixed[time]) {
      continue;
    }

    // Logic to make sure we don't generate states with a cost exceeding
    // max_cost.
    bool already_nonfree;
    if (!state->last_pin.count(thread)) {
      already_nonfree = true;
    } else {
      already_nonfree = state->history.IsSplit(
          state->last_pin[thread],
          state->history.previous_time_of_thread_at(time));
    }
    if (already_nonfree && state->cost == max_cost) {
      continue;
    }

    std::vector<int> conflicts = state->history.first_conflicts_at(time);
    std::vector<ClockVector> cs;
    ClockVector helper_c(999);
    ConsiderPin(state, conflicts.rbegin(), conflicts.rend(), 
        ClockVector(-1), false, helper_c, false,
        state->history.previous_value_at(time), time, max_cost, cs);

    for (ClockVector c : cs) {
      choices.push_back(Choice(time, c));
    }
  }

  return choices;
}

std::map<std::vector<int>, int> cost_histogram_count;

void Explore(PinnerState* state, int max_cost) {
  pinner_states++;

  /*
  int num_threads_with_cost_over_2 = 0;
  for (int thread = 0; thread < kMaxThreads; thread++) {
    if (state->thread_cost[thread] >= 2) {
      num_threads_with_cost_over_2++;
    }
  }

  if (num_threads_with_cost_over_2 >= 2) {
    return;
  }
  */

  std::vector<int> cost_histogram;
  for (int thread = 0; thread < kMaxThreads; thread++) {
    cost_histogram.push_back(state->thread_cost[thread]);
  }
  sort(cost_histogram.begin(), cost_histogram.end());
  cost_histogram_count[cost_histogram]++;

  // Heuristic assumes only pins cost things, not every pinned transition.
  // (in Push: change fixed to is_a_pin to get working heuristic)
  // assert(state->cost <= max_cost);
  if (state->cost > max_cost) {
    return;
  }

  auto choices = GenerateChoices(state, max_cost);
  std::reverse(choices.begin(), choices.end());
  for (Choice choice : choices) {
    int thread = state->history.thread_at(choice.time);
    PinnerState* new_state = GetUnusedState();
    Pin(new_state, choice, state);
    Explore(new_state, max_cost);
    ReturnUnusedState(new_state);
  }
}

