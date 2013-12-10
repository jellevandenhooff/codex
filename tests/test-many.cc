#include "helper.h"

const int N = 4, M = 1;

std::atomic<int> a;

void thread(int tid) {
  a = tid;
}

void Setup() {
  a = 0;
  for (int i = 0; i < N; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("a=%d", a.load());
  Output("\n");
}


