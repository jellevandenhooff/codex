#include "helper.h"

#include <boost/lockfree/fifo.hpp>

// Find ABA in boost/lockfree/fifo at commit 
// 6f9d2d68438c45a66efbe637111157e7c2428f6a
// https://github.com/straszheim/boost.lockfree

// 1: Enqueue observes tail A: next=0-0
// 2: Enqueue inserts element, modifying A: next=B-1 and setting tail to B
// 2: Dequeue removes and frees A, setting head to B: next=0-0
// 2 or 3: Enqueue allocates A: next=0-0
// 1: Enqueue successfully CASes next. CAS of tail fails
// 1: Dequeue spuriously fails.

boost::lockfree::fifo<int> *list;

bool s1, s2;

// DPOR: 14097
// Negative: 14097
void Enqueue(int val) {
  if (val == 1) {
    list->enqueue(val);
    s1 = !list->empty();
  }

  if (val == 2) {
    list->enqueue(val);
    s2 = list->dequeue(&val);
    list->enqueue(val);
  }
}

/*
// Negative: 139720903
void Enqueue(int val) {
  if (val == 1) {
    list->enqueue(val);
    s1 = !list->empty();
  }

  if (val == 2) {
    list->enqueue(val);
    s2 = list->dequeue(&val);
  }

  if (val == 3) {
    list->enqueue(val);
  }
}
*/

void Setup() {
  list = new boost::lockfree::fifo<int>(4);

  s1 = s2 = false;

  StartThread(Enqueue, 3);
  StartThread(Enqueue, 2);
  StartThread(Enqueue, 1);
}

void Finish() {
  if (!s1 || !s2) {
    Output("%d %d\n", s1, s2);
  }
}

