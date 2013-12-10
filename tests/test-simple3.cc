#include "helper.h"

std::atomic<int> x, y, z;

void thread(int i) {
  if (i == 0) {
    if (y == 0) {
      x = 1;
    }
  }

  if (i == 1) {
    x = 2;
  }

  if (i == 2) {
    z = 1;
  }

  if (i == 3) {
    if (z != 1)
      y = 1;
  }
}

void Setup() {
  x = y = z = 0;
  for (int i = 0; i < 4; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("%d %d %d\n", x.load(), y.load(), z.load());
}

