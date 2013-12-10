#include "helper.h"

Mutex lock;
std::atomic<int> x, y;

void thread(int arg) {
  lock.Acquire();
  x = arg;
  y = arg;
  lock.Release();
}

void Setup() {
  lock.Reset();
  x = 0;
  y = 0;
  for (int i = 1; i <= 4; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("x=%d y=%d\n", x.load(), y.load());
}

