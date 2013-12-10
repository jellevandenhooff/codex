#include "helper.h"
#include "linearizability.h"
#include "threadset.h"

#include <boost/lockfree/fifo.hpp>
#include <functional>
#include <vector>
#include <queue>

// Find ABA in boost/lockfree/fifo at commit 
// 6f9d2d68438c45a66efbe637111157e7c2428f6a
// https://github.com/straszheim/boost.lockfree

// 1: Enqueue observes tail A: next=0-0
// 2: Enqueue inserts element, modifying A: next=B-1 and setting tail to B
// 2: Dequeue removes and frees A, setting head to B: next=0-0
// 2 or 3: Enqueue allocates A: next=0-0
// 1: Enqueue successfully CASes next. CAS of tail fails
// 1: Dequeue spuriously fails.
// 2: Enqueue runs again

Linearizability linearizability(2);

boost::lockfree::fifo<int> *list;

void Create() {
  list = new boost::lockfree::fifo<int>(4);
}

void Destroy() {
  while (!list->empty()) {
    int temp;
    list->dequeue(&temp);
  }
  delete list;
}

int Enqueue(int value) {
  list->enqueue(value);
  return 0;
}

int Dequeue() {
  int value;
  return list->dequeue(&value) ? value : -1;
}

struct Configure {
  Configure() {
    linearizability.RegisterImplementation(&Create, &Destroy);
    linearizability.RegisterModel(&Create, &Destroy);
    linearizability.AddStep(0, std::bind(Enqueue, 1), "enqueue 1");
    linearizability.AddStep(0, Dequeue, "dequeue?");
    linearizability.AddStep(1, std::bind(Enqueue, 2), "enqueue 2");
    linearizability.AddStep(1, Dequeue, "dequeue?");
    linearizability.AddStep(1, std::bind(Enqueue, 3), "enqueue 3");
  }
};

static Configure c;

void ThreadBody(int n) {
  linearizability.ThreadBody(n);
}

void Setup() {
  linearizability.Setup();
  for (int i = 0; i < 2; i++) {
    StartThread(&ThreadBody, i);
  }
}

void Finish() {
  linearizability.Finish();
}

