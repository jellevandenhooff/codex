#include "helper.h"

std::atomic<int> x, y;

void thread(int i) {
  if (i == 0) {
    x = 1;
  }

  if (i == 1) {
    y = 1;
  }
  if (i == 2) {
    if (y != 1)
      x = 1;
  }
}

void Setup() {
  x = y = 0;
  for (int i = 0; i < 3; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("%d %d\n", x.load(), y.load());
}

