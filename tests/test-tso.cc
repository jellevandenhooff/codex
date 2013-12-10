#include "helper.h"

const int N = 2, M = 5;

std::atomic<int> x, y, a, b;

void Thread(int i) {
  if (i == 0) {
    x = 1;
    a = y.load();
  } else {
    y = 1;
    b = x.load();
  }
}

void Setup() {
  x = y = a = b = 0;
  for (int i = 0; i < N; i++) {
    TSOStartThread(Thread, i);
  }
}

void Finish() {
  Output("%d %d", a.load(), b.load());
  Output("\n");
}

