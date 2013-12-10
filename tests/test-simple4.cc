#include "helper.h"

std::atomic<int> x, y, z;

int z1, z2;

void thread(int id) {
  if (id == 0) {
    y = 0;
    y = 1;
    y = 2;
    y = 3;
    y = 4;
    int old = 0;
    RequireResult(old);
    while (!z.compare_exchange_weak(old, 1)) {}
  }
  if (id == 1) {
    x.store(y.load());
    x.store(y.load());
  }
  if (id == 2) {
    z1 = x.load();
    z2 = x.load();
  }
}

void Setup() {
  x = 0;
  y = 0;
  z = 0;
  z1 = 0;
  z2 = 0;

  for (int i = 0; i < 3; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("x=%d y=%d z1=%d z2=%d\n", x.load(), y.load(), z1, z2);
}

