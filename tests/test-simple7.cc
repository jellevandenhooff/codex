#include "helper.h"

std::atomic<int> x, y, z;

void thread(int i) {
  if (i == 0) {
    y = 2;
  }

  if (i == 1) {
    z = 1;
    if (x == 1) {
      y = 1;
    }
  }

  if (i == 2) {
    if (z == 1) {
      x = 1;
    }
  }
}

void Setup() {
  x = y = z = 0;
  for (int i = 0; i < 3; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("%d %d %d\n", x.load(), y.load(), z.load());
}


