#include "helper.h"

const int NUMTHREADS = 16;
const int SIZE = 128;
const int MAX = 4;
std::atomic<int> table[SIZE];

void thread(int tid) {
  int m = 0;
  while (true) {
    // w = getmsg
    int w;
    if (m < MAX) {
      w = (++m) * 11 + tid;
    } else {
      return;
    }

    // h = hash(w)
    int h = (w * 7) % SIZE;

    while (true) {
      int old = 0;
      if (table[h].compare_exchange_weak(old, w)) {
        break;
      }
      h = (h + 1) % SIZE;
    }
  }
}

void Setup() {
  for (int i = 0; i < SIZE; i++) {
    table[i] = 0;
  }
  for (int i = 0; i < NUMTHREADS; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("table=");
  for (int i = 0; i < SIZE; i++) {
    Output("%d ", table[i].load());
  }
  Output("\n");
}

