#include "helper.h"

#include <cassert>
#include <map>

std::atomic<int> flag[2], turn, held;

void Thread(int thread) {
  flag[thread] = true;
  TSOBarrier();

  while (flag[1 - thread] == true) {
    if (turn != thread) {
      flag[thread] = false;
      RequireResult(thread);
      while (turn != thread) {}
      flag[thread] = true;
      TSOBarrier();
    }
  }

  // in critical section?

  if (held != 0) {
    Found();
  }

  held = 1;
  held = 0;

  turn = 1 - thread;
  flag[thread] = false;
}

void Setup() {
  flag[0] = flag[1] = turn = held = 0;

  for (int i = 0; i < 2; i++)
    TSOStartThread(Thread, i);
}

void Finish() {
}


