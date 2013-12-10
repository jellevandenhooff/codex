#include "helper.h"

struct node_t {
  node_t* next;
  int32_t value;
} nodes[32];

std::atomic<node_t*> head;

void thread(int arg) {
  node_t* next = &nodes[arg];
  next->value = arg;
  node_t* snapshot = head;
  while (true) {
    next->next = snapshot;
    if (head.compare_exchange_weak(snapshot, next)) {
      break;
    }
  }
}

void Setup() {
  head = NULL;
  for (int i = 1; i <= 5; i++)
    StartThread(thread, i);
}

void Finish() {
  node_t* current = head;
  while (current != NULL) {
    Output("%d -> ", current->value);
    node_t* temp = current;
    current = current->next;
  }
  Output("NULL\n");
}



