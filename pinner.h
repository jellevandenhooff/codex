#pragma once

#include "phhbhistory.h"

struct PinnerState {
  PHHBHistory history;
  std::vector<int> first_seen;
  std::vector<int> last_considered;
  std::vector<bool> fixed;
  std::vector<bool> is_a_pin;
  ThreadMap<int> last_pin;
  int cost;
  ThreadMap<int> thread_cost;
  int depth;
};

struct Choice {
  int time;
  ClockVector c;
  Choice(int time, ClockVector c) : time(time), c(c) {}
};

PinnerState* GetUnusedState();
void ReturnUnusedState(PinnerState* state);

void CreateInitialState(PinnerState* state);
void Pin(PinnerState* state, const Choice& c, const PinnerState* old);
std::vector<Choice> GenerateChoices(PinnerState* state, int max_cost);
void Explore(PinnerState* state, int max_cost);

