#include "helper.h"

// Silly bug in boost/lockfree/atomic_int at commit
// 1bd8759b23bcc4d66848df0ab2d9f99cc5139767
// https://github.com/straszheim/boost.lockfree

volatile int x;

void thread(int arg) {
  while (true) {
    int newx = x+arg;
    if (__sync_bool_compare_and_swap(&x, x, newx)) {
      break;
    }
  }
}

void Setup() {
  x = 0;
  for (int i = 1; i <= 2; i++)
    StartThread(thread, i);
}

void Finish() {
  if (x != 3) {
    Found();
    Output("%d\n", x);
  }
}




