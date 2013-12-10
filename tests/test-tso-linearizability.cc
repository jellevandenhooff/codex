#include "helper.h"
#include "linearizability.h"

Linearizability linearizability(2);

int a, b;

void Create() {
  a = b = 0;
}

void Destroy() {
}

int Read(int* address) {
  return *address;
}

int Write(int* address, int value) {
  *address = value;
  return 0;
}

struct Configure {
  Configure() {
    linearizability.RegisterImplementation(&Create, &Destroy);
    linearizability.RegisterModel(&Create, &Destroy);
    linearizability.AddStep(0, std::bind(Write, &a, 1), "a = 1");
    linearizability.AddStep(0, std::bind(Read, &b), "read b");
    linearizability.AddStep(1, std::bind(Write, &b, 1), "b = 1");
    linearizability.AddStep(1, std::bind(Read, &a), "read a");
  }
};

static Configure c;

void ThreadBody(int n) {
  linearizability.ThreadBody(n);
}

void Setup() {
  linearizability.Setup();
  for (int i = 0; i < 2; i++) {
    TSOStartThread(&ThreadBody, i);
  }
}

void Finish() {
  linearizability.Finish();
}

