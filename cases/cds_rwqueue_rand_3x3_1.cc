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

Linearizability linearizability(3);
cds::container::RWQueue<int>* ds;

void Create() {
  ds = new cds::container::RWQueue<int>();
}

void Destroy() {
  delete ds;
}

struct Configure {
  Configure() {
    linearizability.RegisterImplementation(&Create, &Destroy);
    linearizability.RegisterModel(&Create, &Destroy);
    linearizability.AddStep(0, []() { int x; return ds->dequeue(x) ? x : -1; }, "int x; return ds->dequeue(x) ? x : -1;");
    linearizability.AddStep(0, []() { return ds->empty(); }, "return ds->empty();");
    linearizability.AddStep(0, []() { ds->enqueue(27998); return 0; }, "ds->enqueue(27998); return 0;");
    linearizability.AddStep(1, []() { ds->enqueue(10018); return 0; }, "ds->enqueue(10018); return 0;");
    linearizability.AddStep(1, []() { ds->enqueue(39670); return 0; }, "ds->enqueue(39670); return 0;");
    linearizability.AddStep(1, []() { int x; return ds->dequeue(x) ? x : -1; }, "int x; return ds->dequeue(x) ? x : -1;");
    linearizability.AddStep(2, []() { int x; return ds->dequeue(x) ? x : -1; }, "int x; return ds->dequeue(x) ? x : -1;");
    linearizability.AddStep(2, []() { return ds->empty(); }, "return ds->empty();");
    linearizability.AddStep(2, []() { ds->enqueue(86136); return 0; }, "ds->enqueue(86136); return 0;");
  }
};

static Configure config;

volatile int x;
void ThreadBody(int i) {
  cds::gc::HP::thread_gc gc;
  linearizability.ThreadBody(i);
  int y = x;
}

void Setup() {
  cds::Initialize();
  hpHolder = new cds::gc::HP();
  gcHolder = new cds::gc::HP::thread_gc();
  linearizability.Setup();

  for (int i = 0; i < 3; i++) {
    StartThread(ThreadBody, i);
  }
}

void Finish() {
  linearizability.Finish();
  delete gcHolder;
  delete hpHolder;
  cds::Terminate();
}
