#include <algorithm>
#include <atomic>
#include "helper.h"

//   flush():
//     evict all cache entries (even those with zero deltas)
//     clear cache
//     update the current epoch
//
//   evict(object, delta):
//     object.refcnt <- object.refcnt + delta
//     if object.refcnt = 0:
//       if object is not on any review queue:
//         object.dirty <- false
//         add (object, epoch) to the local review queue
//         if object.weakref
//           object.weakref.dying <- true
//       else:
//         object.dirty <- true
//
//   review():
//     for each (object, oepoch) in local review queue:
//       if oepoch <= epoch + 2:
//         remove object from the review queue
//         if object.refcnt = 0:
//           if object.dirty:
//             evict(object, 0)
//           else if object.weakref and not object.weakref.dying:
//             evict(object, 0)
//           else:
//             if object.weakref:
//               object.weakref.pointer <- null
//               object.weakref.dying <- false
//             free object
//         else:
//           if object.weakref:
//             object.weakref.dying <- false
//
//   get_weakref(weakref):
//     weakref.dying <- false
//     if weakref.pointer:
//       inc(weakref.pointer)
//     return weakref.pointer

static const int kNumThreads = 2;

template<class T>
class Queue {
 public:
  void Reset() {
    start = end = 0;
  }

  const T& front() {
    return buffer[start];
  }

  void push(T x) {
    buffer[end++] = x;
  }

  void pop() {
    start++;
  }

  bool empty() {
    return start == end;
  }

 private:
  T buffer[32];
  int start, end;
};

struct Object {
  void Reset() {
    refcnt = 0;
    dirty = false;
    onqueue = false;
    freed = false;
    lock.Reset();
    delta.Reset();
  }

  RecursiveMutex lock;

  int refcnt;
  bool dirty;
  bool onqueue;
  bool freed;

  ThreadLocalStorage<int> delta;

  void Inc() {
    delta.Get()++;
  }

  void Dec() {
    delta.Get()--;
  }

  bool ShouldEvict() {
    return delta.Get() != 0;
  }

  void Evict();
};

Object the_one_object;

struct Global {
  void Reset() {
    epoch = 0;
    waiters = 0;
  }

  int epoch;
  std::atomic<int> waiters;
};

Global global;

struct Local {
  Local() : review_queue(), epoch(0) {}

  Queue<std::pair<Object*, int>> review_queue;
  int epoch;

  void Review() {
    while (!review_queue.empty() && review_queue.front().second <= epoch - 2) {
      Object* object = review_queue.front().first;
      review_queue.pop();

      //Output("Check by %d on %d\n", ThreadId(), epoch);

      object->lock.Acquire();
      object->onqueue = false;
      if (object->refcnt == 0) {
        if (object->dirty) {
          object->Evict();
        } else {
          object->freed = true;
        }
      }
      object->lock.Release();
    }
  }

  void Flush() {
    if (global.epoch != epoch) {
      return;
    }

    // evict and clear cache
    if (the_one_object.ShouldEvict()) {
      the_one_object.Evict();
    }

    Review();

    epoch++;
    if ((global.waiters += 1) == 2) {
      global.waiters = 0;
      global.epoch++;
    }
  }
};

ThreadLocalStorage<Local> local;

void Object::Evict() {
  lock.Acquire();

  refcnt += delta.Get();
  delta.Get() = 0;

  if (refcnt == 0) {
    if (!onqueue) { // not on any queue,
      dirty = false;
      onqueue = true;
      local.Get().review_queue.push(std::make_pair(this, local.Get().epoch));
      //Output("Pushed by %d on %d\n", ThreadId(), local.Get().epoch);
    } else {
      dirty = true;
    }
  }
  
  lock.Release();
}

bool has_object[2];

void Inc(int id) {
  if (has_object[1 - id]) {
    the_one_object.Inc();
    has_object[id] = true;
  }
}

void Dec(int id) {
  if (has_object[id]) {
    the_one_object.Dec();
    has_object[id] = false;
  }
}

/*
              t ->
      core 0     - * +   |   - * +   |   - * +   |
           1   +       - * +       - * +       - *
      global  1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
      true    1 2 1 1 2 1 1 2 1 1 2 1 1 2 1 1 2 1 1
      epoch   ^-----1----^-----2-----^-----3-----^
*/

void Run(int id) {
  local.Get().review_queue.Reset();

  if (id == 0) {
    Dec(id);
    local.Get().Flush();
    Inc(id);
    Dec(id);
    local.Get().Flush();
    Inc(id);
    Dec(id);
    local.Get().Flush();
    Inc(id);
  } else {
    Inc(id);
    Dec(id);
    local.Get().Flush();
    Inc(id);
    Dec(id);
    local.Get().Flush();
    Inc(id);
    Dec(id);
    local.Get().Flush();
  }
}

void Setup() {
  global.Reset();
  local.Reset();

  the_one_object.Reset();
  the_one_object.refcnt = 1;

  has_object[0] = true;
  has_object[1] = false;

  StartThread(Run, 0);
  StartThread(Run, 1);
}

void Finish() {
  //Output("%d %d %d %d\n", global.epoch, the_one_object.freed, has_object[0], has_object[1]);
  if (the_one_object.freed && (has_object[0] || has_object[1])) {
    Found();
  }
}

