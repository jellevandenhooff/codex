#include "helper.h"
#include "linearizability.h"

#include <cds/container/treiber_stack.h>
#include <cds/container/basket_queue.h>
#include <cds/container/moir_queue.h>
#include <cds/container/msqueue.h>
#include <cds/container/optimistic_queue.h>
#include <cds/container/rwqueue.h>
#include <cds/container/vyukov_mpmc_cycle_queue.h>
#include <cds/container/lazy_list_hp.h>
#include <cds/container/skip_list_set_hp.h>
#include <boost/lockfree/fifo.hpp>

#include <cds/init.h>
#include <cds/gc/hp.h>

cds::gc::HP *hpHolder;
cds::gc::HP::thread_gc *gcHolder;

ThreadLocalStorage<cds::threading::ThreadData*> cdsTLS;

namespace cds {
  CDS_ATOMIC::atomic<size_t> threading::ThreadData::s_nLastUsedProcNo(0);
  size_t threading::ThreadData::s_nProcCount = 1;

  namespace details {
    bool init_first_call() {
      return true;
    }

    bool fini_last_call() {
      return true;
    }
  }
}

Linearizability linearizability(5);
boost::lockfree::fifo<int>* ds;

void Create() {
  ds = new boost::lockfree::fifo<int>();
}

void Destroy() {
  while (!ds->empty()) {
    int temp;
    ds->dequeue(&temp);
  }
  delete ds;
}

struct Configure {
  Configure() {
    linearizability.RegisterImplementation(&Create, &Destroy);
    linearizability.RegisterModel(&Create, &Destroy);
    linearizability.AddStep(0, []() { ds->enqueue(60977); return 0; }, "ds->enqueue(60977); return 0;");
    linearizability.AddStep(1, []() { int x; return ds->dequeue(&x) ? x : -1; }, "int x; return ds->dequeue(&x) ? x : -1;");
    linearizability.AddStep(2, []() { ds->enqueue(21877); return 0; }, "ds->enqueue(21877); return 0;");
    linearizability.AddStep(3, []() { ds->enqueue(34022); return 0; }, "ds->enqueue(34022); return 0;");
    linearizability.AddStep(4, []() { int x; return ds->dequeue(&x) ? x : -1; }, "int x; return ds->dequeue(&x) ? x : -1;");
  }
};

static Configure config;

volatile int x;
void ThreadBody(int i) {
  linearizability.ThreadBody(i);
  int y = x;
}

void Setup() {
  linearizability.Setup();

  for (int i = 0; i < 5; i++) {
    StartThread(ThreadBody, i);
  }
}

void Finish() {
  linearizability.Finish();
}
