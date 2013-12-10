#pragma once

#include <functional>
#include <string>
#include <vector>

#include "clockvector.h"

struct Ordering {
  int thread;
  int actual_thread;
  int function;
  int result;
  ClockVector start_cv;
  ClockVector end_cv;
  bool executed;

  Ordering() {}
};

class Linearizability {
 public:
  Linearizability(int num_threads);

  void RegisterModel(std::function<void()> setup, std::function<void()> cleanup);
  void RegisterImplementation(std::function<void()> setup, std::function<void()> cleanup);

  void AddStep(int thread, std::function<int()> function, std::string name);
  void Setup();
  void Finish();
  void ThreadBody(int thread);

 private:
  bool Verify();
  bool Search();

  std::vector<std::vector<std::pair<std::function<int()>, std::string>>> threads;
  std::function<void()> setup_model, cleanup_model, setup_impl, cleanup_impl;

  std::vector<Ordering> order;
  std::vector<int> linearization;
};

