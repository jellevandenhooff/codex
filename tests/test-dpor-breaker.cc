#include "helper.h"

const int N = 6, M = 6;
std::atomic<int> a[N], b[M], done;

void Waiter(int i) {
  if (done)
    a[i] = 1;
}

void Worker(int i) {
  b[i] = 1;
}

void Watcher(int i) {
  int all = 1;
  for (int i = 0; i < M; i++) {
    all = all & b[i];
  }
  if (all) {
    done = 1;
  }
}

void Setup() {
  for (int i = 0; i < N; i++) {
    a[i] = 0;
    StartThread(Waiter, i);
  }

  for (int i = 0; i < M; i++) {
    b[i] = 0;
    StartThread(Worker, i);
  }

  done = 0;
  StartThread(Watcher, 0);
}

void Finish() {
  for (int i = 0; i < N; i++)
    Output("%d ", a[i].load());
  for (int i = 0; i < M; i++)
    Output("%d ", b[i].load());
  Output("%d", done.load());
  Output("\n");
}

