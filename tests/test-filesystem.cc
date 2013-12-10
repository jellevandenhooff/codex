#include "helper.h"

const int NUMTHREADS = 20;
const int NUMBLOCKS = 26;
const int NUMINODE = 32;
Mutex locki[NUMINODE];
std::atomic<int> inode[NUMINODE];
Mutex lockb[NUMBLOCKS];
std::atomic<int> busy[NUMBLOCKS];

void thread(int tid) {
  int i = tid % NUMINODE;
  locki[i].Acquire();
  if (inode[i] == 0) {
    int b = (i * 2) % NUMBLOCKS;
    while (true) {
      lockb[b].Acquire();
      if (!busy[b]) {
        busy[b] = true;
        inode[i] = b + 1;
        lockb[b].Release();
        break;
      }
      lockb[b].Release();
      b = (b + 1) % NUMBLOCKS;
    }
  }
  locki[i].Release();
}

void Setup() {
  for (int i = 0; i < NUMINODE; i++) {
    locki[i].Reset();
    inode[i] = 0;
  }
  for (int i = 0; i < NUMBLOCKS; i++) {
    lockb[i].Reset();
    busy[i] = false;
  }
  for (int i = 0; i < NUMTHREADS; i++)
    StartThread(thread, i);
}

void Finish() {
  Output("inode=");
  for (int i = 0; i < NUMINODE; i++) {
    Output("%d ", inode[i].load());
  }
  Output("\n");
  Output("busy=");
  for (int i = 0; i < NUMBLOCKS; i++) {
    Output("%d ", busy[i].load());
  }
  Output("\n");
}

