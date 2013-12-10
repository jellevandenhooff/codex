#include "helper.h"
#include "linearizability.h"
#include "../tests/test-crange.cc"
Linearizability linearizability(3);
crange* ds;

void Create() {
  seed_tls.Reset();
  ds = new crange(8);
}

void Destroy() {
  delete ds;
}

struct Configure {
  Configure() {
    linearizability.RegisterImplementation(&Create, &Destroy);
    linearizability.RegisterModel(&Create, &Destroy);
    linearizability.AddStep(0, []() { return ds->search(1, 1, 0) != nullptr ? 1 : 0; }, "return ds->search(1, 1, 0) != nullptr ? 1 : 0;");
    linearizability.AddStep(0, []() { ds->add(1, 1, (void*) 1); return 0; }, "ds->add(1, 1, (void*) 1); return 0;");
    linearizability.AddStep(0, []() { return ds->search(1, 1, 0) != nullptr ? 1 : 0; }, "return ds->search(1, 1, 0) != nullptr ? 1 : 0;");
    linearizability.AddStep(1, []() { ds->del(1, 1); return 0; }, "ds->del(1, 1); return 0;");
    linearizability.AddStep(1, []() { ds->del(1, 1); return 0; }, "ds->del(1, 1); return 0;");
    linearizability.AddStep(1, []() { return ds->search(1, 1, 0) != nullptr ? 1 : 0; }, "return ds->search(1, 1, 0) != nullptr ? 1 : 0;");
    linearizability.AddStep(2, []() { ds->del(1, 1); return 0; }, "ds->del(1, 1); return 0;");
    linearizability.AddStep(2, []() { ds->add(1, 1, (void*) 1); return 0; }, "ds->add(1, 1, (void*) 1); return 0;");
    linearizability.AddStep(2, []() { ds->del(1, 1); return 0; }, "ds->del(1, 1); return 0;");
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

  for (int i = 0; i < 3; i++) {
    StartThread(ThreadBody, i);
  }
}

void Finish() {
  linearizability.Finish();
}
