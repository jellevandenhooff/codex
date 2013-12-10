#include "helper.h"

std::atomic<int8_t> i8;
std::atomic<int16_t> i16;
std::atomic<int32_t> i32;
std::atomic<int64_t> i64;
std::atomic<int*> ptr;

void Worker(int arg) {
  ptr = &arg;
  i64 = 1ll << 60;
  i32 = 1ll << 30;
  i16 = 1ll << 10;
  i8 = 120;
  ptr = NULL;
}

void Waiter(int arg) {
  RequireResult(1ll << 60);
  while (i64 != (1ll << 60)) {}

  RequireResult(1ll << 30);
  while (i32 != (1ll << 30)) {}

  RequireResult(1ll << 10);
  while (i16 != (1ll << 10)) {}

  RequireResult(120);
  while (i8 != 120) {}

  RequireResult(NULL);
  while (ptr != NULL) {}
}

void Setup() {
  i8 = 0;
  i16 = 0;
  i32 = 0;
  i64 = 0;
  ptr = NULL;
  StartThread(Waiter, 0);
  StartThread(Worker, 0);
}

void Finish() {
}

