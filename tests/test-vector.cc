#include "helper.h"

#include <vector>

// This test expectedly causes assertions as vector isn't threadsafe.

std::vector<int> v;

void thread(int arg) {
  v.push_back(arg);
}

void Setup() {
  // Reset v to be empty
  std::vector<int> tmp;
  std::swap(v, tmp);
  for (int i = 1; i <= 3; i++)
    StartThread(thread, i);
}

void Finish() {
  for (int i = 0; i < v.size(); i++) {
    Output("v[%d] = %d\n", i, v[i]);
  }
}

