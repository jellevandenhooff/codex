#include "helper.h"

std::atomic<int> x;

void thread(int i) {
  x = i;
}

void Setup() {
  x = 0;
  for (int i = 0; i < 3; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("%d\n", x.load());
}


