#include <cstdio>

#include <map>
#include <random>
#include <string>
#include <vector>

#include "codex_interface.h"
#include "interceptor.h"
#include "hhbhistory.h"
#include "pinner.h"
#include "statistics.h"
#include "trace_builder.h"


bool show_all_transitions = false;
bool show_program_output = false;
bool show_debug_output = false;

Interceptor* interceptor;
TraceBuilder* trace_builder;
HHBHistory* history;

std::vector<ThreadSet> available, backtrack;
std::vector<int> begins;

ThreadSet FindConflicts(
    ThreadMap<Transition> transitions, Transition transition) {
  ThreadSet conflicts;
  for (int thread : transitions.keys()) {
    if (transitions[thread].ConflictsWith(transition)) {
      conflicts.insert(thread);
    }
  }
  return conflicts;
}

int64_t& dpor_leaves = RegisterStatistic<int64_t>("dpor-leaves");
int64_t& dpor_deadends = RegisterStatistic<int64_t>("dpor-deadends");

void DPORExplore(std::shared_ptr<TraceNode> node, ThreadSet sleepset) {
  if (node->is_leaf()) {
    dpor_leaves++;
    return;
  }

  available.push_back(node->runnable() - sleepset);
  if (available.back().empty()) {
    available.pop_back();
    dpor_deadends++;
    return;
  }

  backtrack.push_back(ThreadSet());
  if (node->parent() && available.back().count(node->last_thread())) {
    backtrack.back().insert(node->last_thread());
  } else {
    backtrack.back().insert(*available.back().begin());
  }

  ThreadSet done;
  while (true) {
    ThreadSet todo = backtrack.back() - done;
    if (todo.empty()) {
      break;
    }

    int thread = *todo.begin();
    Transition transition = node->next_transitions()[thread];

    trace_builder->MoveTo(node);
    for (int time : history->FindFirstConflicts(thread, transition)) {
      if (transition.DetermineRunnable(history->previous_value_at(time))) {
        if (available[time].count(thread)) {
          backtrack[time].insert(thread);
        } else {
          backtrack[time] = backtrack[time] | available[time];
        }
      }
    }

    ThreadSet new_sleepset = 
      sleepset - FindConflicts(node->next_transitions(), transition);

    DPORExplore(trace_builder->Extend(thread), new_sleepset);
    sleepset.insert(thread);
    done.insert(thread);
  }

  available.pop_back();
  backtrack.pop_back();
}

void RunDPOR() {
  trace_builder = new TraceBuilder(interceptor, history);
  DPORExplore(trace_builder->root(), ThreadSet());
  DumpStatisticsToStderr();
}

int64_t& bpor_leaves = RegisterStatistic<int64_t>("bpor-leaves");
int64_t& bpor_deadends = RegisterStatistic<int64_t>("bpor-deadends");

void PBPORBacktrack(int time, int thread) {
  if (available[time].count(thread)) {
    backtrack[time].insert(thread);
  } else {
    backtrack[time] = backtrack[time] | available[time];
  }
}

void PBPORExplore(
    std::shared_ptr<TraceNode> node, ThreadSet sleepset, int remaining) {
  if (node->is_leaf()) {
    bpor_leaves++;
    return;
  }

  available.push_back(node->runnable() - sleepset);
  if (available.back().empty()) {
    available.pop_back();
    bpor_deadends++;
    return;
  }

  backtrack.push_back(ThreadSet());
  if (node->parent() && available.back().count(node->last_thread())) {
    backtrack.back().insert(node->last_thread());
  } else {
    backtrack.back().insert(*available.back().begin());
  }

  ThreadSet done;
  while (true) {
    ThreadSet todo = backtrack.back() - done;
    if (todo.empty()) {
      break;
    }

    int thread = *todo.begin();
    Transition transition = node->next_transitions()[thread];

    bool is_a_preemption = node->parent() && thread != node->last_thread() &&
      node->runnable().count(node->last_thread());
    if (is_a_preemption && !remaining) {
      done.insert(thread);
      continue;
    }

    trace_builder->MoveTo(node);
    for (int time : history->FindFirstConflicts(thread, transition)) {
      if (transition.DetermineRunnable(history->previous_value_at(time))) {
        PBPORBacktrack(time, thread);
        PBPORBacktrack(begins[time], thread);
      }
    }

    ThreadSet new_sleepset = 
      sleepset - FindConflicts(node->next_transitions(), transition);

    if (!node->parent() || node->last_thread() != thread) {
      begins.push_back(history->length());
    } else {
      begins.push_back(begins.back());
    }

    PBPORExplore(trace_builder->Extend(thread), new_sleepset,
        remaining - is_a_preemption);

    begins.pop_back();

    if (is_a_preemption) {
      sleepset.insert(thread);
    }
    done.insert(thread);
  }

  available.pop_back();
  backtrack.pop_back();
}

void RunPBPOR() {
  trace_builder = new TraceBuilder(interceptor, history);
  for (int preemptions = 0;; preemptions++) {
    PBPORExplore(trace_builder->root(), ThreadSet(), preemptions);
    DumpStatisticsToStderr();
  }
}


int64_t& cbdpor_leaves = RegisterStatistic<int64_t>("cbdpor-leaves");
int64_t& cbdpor_deadends = RegisterStatistic<int64_t>("cbdpor-deadends");

void CBDPORExplore(
    std::shared_ptr<TraceNode> node, ThreadSet sleepset, int remaining) {
  if (node->is_leaf()) {
    cbdpor_leaves++;
    return;
  }

  available.push_back(node->runnable() - sleepset);
  if (available.back().empty()) {
    available.pop_back();
    cbdpor_deadends++;
    return;
  }

  backtrack.push_back(ThreadSet());
  if (node->parent() && available.back().count(node->last_thread())) {
    backtrack.back().insert(node->last_thread());
  } else {
    backtrack.back() = available.back();
  }

  ThreadSet done;
  while (true) {
    ThreadSet todo = backtrack.back() - done;
    if (todo.empty()) {
      break;
    }

    int thread = *todo.begin();
    Transition transition = node->next_transitions()[thread];

    bool is_a_preemption = node->parent() && thread != node->last_thread() &&
      node->runnable().count(node->last_thread());
    if (is_a_preemption && !remaining) {
      done.insert(thread);
      continue;
    }

    trace_builder->MoveTo(node);
    for (int time : history->FindFirstConflicts(thread, transition)) {
      if (transition.DetermineRunnable(history->previous_value_at(time))) {
        backtrack[time] = available[time];
      }
    }

    ThreadSet new_sleepset = 
      sleepset - FindConflicts(node->next_transitions(), transition);

    if (!node->parent() || node->last_thread() != thread) {
      begins.push_back(history->length());
    } else {
      begins.push_back(begins.back());
    }

    CBDPORExplore(trace_builder->Extend(thread), new_sleepset,
        remaining - is_a_preemption);

    begins.pop_back();

    if (is_a_preemption) {
      sleepset.insert(thread);
    }
    done.insert(thread);
  }

  available.pop_back();
  backtrack.pop_back();
}

void RunCBDPOR() {
  trace_builder = new TraceBuilder(interceptor, history);
  for (int preemptions = 0;; preemptions++) {
    CBDPORExplore(trace_builder->root(), ThreadSet(), preemptions);
    DumpStatisticsToStderr();
  }
}

void BruteForceExplore(std::shared_ptr<TraceNode> node) {
  if (node->is_leaf()) {
    trace_builder->MoveTo(node);
    return;
  }

  for (int thread : node->runnable()) {
    trace_builder->MoveTo(node);
    BruteForceExplore(trace_builder->Extend(thread));
  }
}

void RunBruteForce() {
  trace_builder = new TraceBuilder(interceptor, history);
  BruteForceExplore(trace_builder->root());
  DumpStatisticsToStderr();
}

static std::map<Hash, int> seen;
static bool prune_using_hash_table = false;
static bool only_preempt_on_atomic = false;

void CHESSExplore(std::shared_ptr<TraceNode> node, int remaining) {
  if (node->is_leaf()) {
    return;
  }

  if (prune_using_hash_table) {
    // Prune traces already seen.
    Hash hash = history->CombineCurrentHashesWithLast();
    auto it = seen.insert(std::make_pair(hash, remaining));
    if (!it.second) {
      if (it.first->second >= remaining) {
        return;
      } else {
        it.first->second = remaining;
      }
    }
  }

  // Extend this node
  for (int thread : node->runnable()) {
    bool is_a_preemption = node->parent() && thread != node->last_thread() &&
      node->runnable().count(node->last_thread());

    if (is_a_preemption && !remaining) {
      continue;
    }

    if (only_preempt_on_atomic) {
      if (is_a_preemption && 
          node->next_transitions()[node->last_thread()].is_atomic()) {
        continue;
      }
    }

    trace_builder->MoveTo(node);
    CHESSExplore(trace_builder->Extend(thread), remaining - is_a_preemption);
  }
}

void RunCHESS() {
  trace_builder = new TraceBuilder(interceptor, history);
  for (int preemptions = 0;; preemptions++) {
    CHESSExplore(trace_builder->root(), preemptions);
    DumpStatisticsToStderr();
  }
}

void RunSingle() {
  interceptor->StartNewRun(history);
  while (!interceptor->finished()) {
    interceptor->AdvanceThread(*interceptor->runnable().begin());
  }
  DumpStatisticsToStderr();
}

static std::mt19937_64 prng(0);
static int& max_program_length = RegisterStatistic("max-program-length", -1);

static int HighestPriorityThread(ThreadMap<int> priority, ThreadSet runnable) {
  int best_thread = -1, best_priority = -1;
  for (int thread : runnable) {
    if (priority[thread] > best_priority) { 
      best_thread = thread;
      best_priority = priority[thread];
    }
  }
  return best_thread;
}

void PCTOnce(int num_changes, int max_program_length) {
  ThreadMap<int> priority;
  for (int i = 0; i < kMaxThreads; i++) {
    priority[i] = num_changes + i;
  }
  for (int i = 0; i < kMaxThreads; i++) {
    std::swap(priority[i], 
        priority[std::uniform_int_distribution<int>(0, i)(prng)]);
  }

  std::vector<std::pair<int, int>> changes;
  for (int i = 0; i < num_changes; i++) {
    changes.push_back(std::make_pair(
        std::uniform_int_distribution<int>(0, max_program_length)(prng), i));
  }
  std::sort(changes.begin(), changes.end());

  auto change = changes.begin();

  interceptor->StartNewRun(history);
  while (!interceptor->finished()) {
    while (change != changes.end() && change->first == history->length()) {
      int thread = HighestPriorityThread(priority, interceptor->runnable());
      priority[thread] = change->second;
      change++;
    }
    int thread = HighestPriorityThread(priority, interceptor->runnable());
    interceptor->AdvanceThread(thread);
  }
}

void RunPCT() {
  interceptor->StartNewRun(history);
  int num_threads = interceptor->next_transitions().size();
  int num_changes = 10;

  max_program_length = 0;

  for (int i = 1; ; i++) {
    PCTOnce(num_changes, max_program_length);

    max_program_length = std::max(max_program_length, history->length());

    double p = 1.0 / num_threads / pow(max_program_length, num_changes);

    double required_runs;
    if (p < 1e-10) {
      required_runs = 1e10;
    } else {
      required_runs = log(0.01) / log(1 - p);
    }

    if (i > required_runs) {
      break;
    }

    if (i % 1000 == 0) {
      DumpStatisticsToStderr();
    }
  }
  DumpStatisticsToStderr();
}

extern std::map<std::vector<int>, int> cost_histogram_count;

void DumpHistogram() {
  for (auto it : cost_histogram_count) {
    fprintf(stderr, "Histogram:");
    for (auto value : it.first) {
      fprintf(stderr, " %d", value);
    }
    fprintf(stderr, " x %d\n", it.second);
  }
}

void DumpChoice(const Choice& choice) {
  fprintf(stderr, "time=%d c=", choice.time);
  for (int j = 0; j < kMaxThreads; j++) {
    fprintf(stderr, "%d ", choice.c[j]);
  }
  fprintf(stderr, "\n");
}

void RunPinner() {
  PinnerState* root = GetUnusedState();
  for (int cost = 0;; cost++) {
    CreateInitialState(root);
    cost_histogram_count.clear();
    Explore(root, cost);
    DumpStatisticsToStderr();
    DumpHistogram();
    int total_not_exceeding_cost;
    for (auto it : cost_histogram_count) {
      int actual_cost = 0;
      for (auto value : it.first) {
        actual_cost += value;
      }
      if (actual_cost <= cost) {
        total_not_exceeding_cost += it.second;
      }
    }
    fprintf(stderr, "Total runs not exceeding cost: %d\n", total_not_exceeding_cost);
  }
}

void RunPinnerInteractive() {
  PinnerState* state = GetUnusedState();
  CreateInitialState(state);
  while (true) {
    printf("Cost: %d\n", state->cost);
    state->history.Dump();
    DumpStatisticsToStderr();

    cost_histogram_count.clear();

    auto choices = GenerateChoices(state, 10);

    DumpHistogram();

    for (int i = 0; i < choices.size(); i++) {
      fprintf(stderr, "[% 3d] ", i);
      DumpChoice(choices[i]);
    }

    int idx;
    scanf("%d", &idx);
    auto choice = choices[idx];
    fprintf(stderr, "Running ");
    DumpChoice(choice);

    PinnerState* tmp = GetUnusedState();
    Pin(tmp, choice, state);
    std::swap(state, tmp);
    ReturnUnusedState(tmp);
  }
}

int main(int argc, char** argv) {
  //show_all_transitions = true;
  //show_debug_output = true;
  //show_program_output = true;

  interceptor = SetupInterfaceAndInterceptor();
  history = new HHBHistory();

  //RunSingle();
  //RunBruteForce();
  //RunCHESS();
  //RunPBPOR();
  //RunPCT();
  //RunPinner();
  //RunDPOR();
  RunCBDPOR();

  //RunPinnerInteractive();
}

