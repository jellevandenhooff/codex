#include "helper.h"

std::atomic<int> x;

void CAS(int a, int b) {
  RequireResult(a);
  while (!x.compare_exchange_weak(a, b)) {}
}

void thread(int i) {
  if (i == 0) {
    CAS(0, 1);
  }
  if (i == 1) {
    CAS(1, 2);
  }
  if (i == 2) {
    CAS(2, 0);
  }
  if (i == 3) {
    CAS(0, 3);
  }
  if (i == 4) {
    CAS(3, 0);
  }
}

void Setup() {
  for (int i = 0; i < 5; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("%d\n", x.load());
}

