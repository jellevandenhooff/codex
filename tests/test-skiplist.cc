#include "helper.h"
#include "linearizability.h"

#include <atomic>

#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>

#include <cmath>

#include <cstdlib>
#include <ctime>
#include <cassert>

#include <unistd.h>
#include <sys/time.h>

// Just for testing
bool thread_done[64];

// concurrent lock free skip list implementation based on
// The Art of Multiprocessor Programming - Herlihy and Shavit

// assumes x86_64 architecture
#if !defined(__x86_64__)
#error "Assumes x86_64 architecture"
#endif

#ifndef ALWAYS_ASSERT
#ifdef NDEBUG
#define ALWAYS_ASSERT(x)  \
  do { \
    if (!(x)) { \
      Output("always_assertion failure: %s\n", #x); \
      exit(1); \
    } \
  } while (0)
#else
#define ALWAYS_ASSERT(x) assert(x)
#endif
#endif

#define ATTR_UNUSED __attribute__ ((unused))

#define LIKELY(pred)   __builtin_expect(!!(pred), true)
#define UNLIKELY(pred) __builtin_expect(!!(pred), false)

struct ptr_ops {
 private:
  // lower 6 bytes
  static const intptr_t PtrMask       = 0xFFFFFFFFFFFFL;
  static const intptr_t PtrOn   = 0x1000000000000L;
  static const intptr_t PtrLastBit    = 0x800000000000L;
  static const intptr_t PtrUnusedMask = 0xFFFF000000000000L;
 public:
  static inline bool ptr_is_mark_set(intptr_t p) {
    return ((intptr_t(p) & PtrOn));
  }
  static inline intptr_t ptr_set_mark(intptr_t p, bool mark) {
    return (mark ? ((intptr_t(p) & PtrMask) | PtrOn) : (intptr_t(p) & PtrMask));
  }
  static inline intptr_t ptr_restore(intptr_t p) {
    intptr_t x = intptr_t(p) & PtrMask;
    // extend the last bit
    return ((x & PtrLastBit) ? (x | PtrUnusedMask) : x);
  }
};

class rcu {
 private:
  template <typename T>
  struct deleter {
    static void do_delete(void* p) {
      delete (T*) p;
    }
  };

 public:
  static void rcu_begin_critical_section() {
    rcu_state* s = get_thread_local_state();
    s->_mutex.Acquire();
    s->_epoch_depth++;
    s->_mutex.Release();
  }

  static void rcu_end_critical_section() {
    rcu_state* s = get_thread_local_state();
    s->_mutex.Acquire();
    s->_epoch_depth--;
    s->_mutex.Release();
  }

  static void rcu_delayed_free(void* p, void (*deleter)(void*)) {
    rcu_state* s = get_thread_local_state();
    s->_mutex.Acquire();
    assert(s->_epoch_depth); // must be in RCU critical section
    size_t queue_idx = s->_current_epoch % 4;
    s->_delay_queues[queue_idx].push_back(std::make_pair(p, deleter));

    s->_mutex.Release();
  }

  template <typename T>
  static void rcu_delayed_free(T* p) {
    rcu_delayed_free(p, deleter<T>::do_delete);
  }

  static bool rcu_in_critical_section() {
    rcu_state* s = get_thread_local_state();
    s->_mutex.Acquire();
    bool result = s->_epoch_depth;
    s->_mutex.Release();
    return s->_epoch_depth;
  }

  struct scoped_critical_section {
    scoped_critical_section() {
      rcu_begin_critical_section();
    }
    ~scoped_critical_section() {
      rcu_end_critical_section();
    }
  };

 public:

  // XXX: hack for now- all exiting threads which use RCU must call this
  // function in order to exit cleanly
  static void rcu_thread_cleanup() {
    assert(_gc_thread);
    rcu_state*& rs = _rcu_state_thd_local.Get();
    if (rs != NULL) {
      _global_lock->Acquire();
      _rcu_thread_cleanup(rs);
      delete rs;
      rs = NULL;
      _global_lock->Release();
    }
  }

  // start RCU gc thread- should be done by a single thread
  static void rcu_start_gc_thread() {
    assert(!_gc_thread_keep_running.load());
    _gc_thread_keep_running.store(true);

    assert(!_global_lock);
    _global_lock = new Mutex();

    assert(_global_epoch == 0);

    assert(!_gc_thread);
    // TODO
    _gc_thread = new int(StartThread(garbage_collector_thread, 0));
  }

  // should be called by a single thread
  static void rcu_stop_gc_thread() {
    assert(_gc_thread_keep_running.load());
    _gc_thread_keep_running.store(false);

    bool *ptr = &thread_done[*_gc_thread];
    RequireResult(true);
    while (*ptr != true) {}
    delete _gc_thread;
    _gc_thread = NULL;

    // delete remaining elems in global delay queue
    for (size_t i = 0; i < 4; i++)
      purge_and_clear_delay_queue(_global_delay_queues[i]);

    delete _global_lock;
    _global_lock = NULL;


    // Reset thread states to an empty vector
    std::vector< rcu_state* > tmp;
    std::swap(_thread_states, tmp);
    
    _global_epoch = 0; // XXX: do we need some atomic here?
  }

 private:

  static void garbage_collector_thread(int) {
    // the primary GC thread. is responsible for advancing global epochs

    assert(_global_epoch == 0); // gc thread should start w/ _global_epoch == 0

    bool can_advance_epoch = true;
    while (_gc_thread_keep_running.load()) {
      //RequestYield(0);
      continue;

      if (can_advance_epoch) {
        _global_epoch++; // advance epoch
      }

      {
        // try to catch the threads up to _global_epoch
        _global_lock->Acquire();
        size_t advanced = 0;
        for (auto s : _thread_states) {
          // assert at most 1 epoch behind
          assert((s->_current_epoch == _global_epoch) ||
                 (s->_current_epoch + 1 == _global_epoch));
          if (s->_current_epoch < _global_epoch) {
            for (size_t i = 0; i < 1; i++) {
              if (s->_mutex.TryAcquire()) {
                if (s->_epoch_depth == 0) {
                  s->_current_epoch = _global_epoch;
                  advanced++;

                  s->_mutex.Release();
                  break;
                }
                s->_mutex.Release();
              }
              //RequestYield(0);
            }
          } else {
            // was already on current epoch
            advanced++;
          }
        }

        if (advanced == _thread_states.size()) {
          // all threads caught up, so we can advance to the next epoch
          can_advance_epoch = true;

          // run GC for global_epoch - 2.
          // TODO: run this GC in the background
          if (_global_epoch >= 2) {
            uint64_t gc_epoch = _global_epoch - 2;
            for (auto s : _thread_states) {
              purge_and_clear_delay_queue(s->_delay_queues[gc_epoch % 4]);
            }
            purge_and_clear_delay_queue(_global_delay_queues[gc_epoch % 4]);
          }
        } else {
          // still waiting for some threads to finish the previous epoch,
          // so we cannot advance the epoch again
          can_advance_epoch = false;
        }

        _global_lock->Release();
      }

      //RequestYield(0);
    }

    _global_lock->Acquire();

    if (!_thread_states.empty()) {
      std::cerr << "WARNING: not all threads cleaned up state! "
          << "remaining pointers are leaked" << std::endl;
      _thread_states.clear();
    }

    _global_lock->Release();

    thread_done[ThreadId()] = true;
  }

  typedef std::vector< std::pair< void*, void(*)(void*) > > delay_queue;

  struct rcu_state {
    rcu_state(uint64_t current_epoch)
      : _current_epoch(current_epoch),
        _epoch_depth(0),
        _mutex()
    { }

    uint64_t _current_epoch; // current epoch
    uint64_t _epoch_depth; // allows for re-entrant rcu critical sections

    RecursiveMutex _mutex;

    // delay queues, one for the cur epoch, one for the prev
    delay_queue _delay_queues[4];
  };

  // assumes the _global_lock is held
  static void _rcu_thread_cleanup(rcu_state* rs)
  {
    bool found = false;
    for (auto it = _thread_states.begin(); it != _thread_states.end(); ) {
      if ((*it) == rs) {
        found = true;
        it = _thread_states.erase(it);
      } else {
        ++it;
      }
    }
    if (!found) {
      std::cerr << "WARNING: thread could not find local state pointer "
          << "in global thread states" << std::endl;
    }

    // add remaining elemens in delay queue to global delay queue
    for (size_t i = 0; i < 4; i++)
      _global_delay_queues[i].insert(
        _global_delay_queues[i].end(),
        rs->_delay_queues[i].begin(),
        rs->_delay_queues[i].end());
  }

  static void purge_and_clear_delay_queue(delay_queue& queue) {
    for (auto p : queue) p.second(p.first);
    queue.clear();

    // Reset internal state
    delay_queue tmp;
    std::swap(queue, tmp);
  }

  static rcu_state* get_thread_local_state() {
    assert(_gc_thread);
    rcu_state*& rs = _rcu_state_thd_local.Get();
    if (rs == NULL) {
      // add to global list
      _global_lock->Acquire();
      rs = new rcu_state(_global_epoch);
      _thread_states.push_back(rs);
      _global_lock->Release();
    }
    return rs;
  }

  static int* _gc_thread;
  // Used to be an untracked atomic bool.
  static std::atomic<bool> _gc_thread_keep_running;
  static uint64_t _global_epoch;

  static Mutex* _global_lock; // used only to manage the list of all active RCU threads, and the global delay queue

  static std::vector< rcu_state* > _thread_states;

  // protected by _global_lock
  static delay_queue _global_delay_queues[4];

 public:
  static ThreadLocalStorage<rcu_state*> _rcu_state_thd_local;
};

ThreadLocalStorage<rcu::rcu_state*> rcu::_rcu_state_thd_local;

int* rcu::_gc_thread = NULL;
std::atomic<bool> rcu::_gc_thread_keep_running(false);
uint64_t rcu::_global_epoch = 0;

Mutex* rcu::_global_lock = NULL;
std::vector< rcu::rcu_state* > rcu::_thread_states;
rcu::delay_queue rcu::_global_delay_queues[4];

template <typename Ptr>
class ref_mark_ptr {
 private:
  static inline Ptr* cast(intptr_t x) {
    return reinterpret_cast<Ptr*>(x);
  }
 public:
  explicit ref_mark_ptr()
    : _ptr(ptr_ops::ptr_set_mark(0, false)) {}
  explicit ref_mark_ptr(Ptr *ptr, bool mark)
    : _ptr(ptr_ops::ptr_set_mark(ptr, mark)) {}

 private:
  inline Ptr* get_raw() const {
    return cast(ptr_ops::ptr_restore(_ptr.load()));
  }

 public:
  ref_mark_ptr(const ref_mark_ptr<Ptr>& that)
    : _ptr(that._ptr) {}

  inline Ptr* get() const {
    return get_raw();
  }

  inline Ptr* get(bool& mark) const {
    intptr_t x = _ptr.load();
    mark = ptr_ops::ptr_is_mark_set(x);
    return cast(ptr_ops::ptr_restore(x));
  }

  void set(Ptr* ptr, bool mark) {
    intptr_t x = ptr_ops::ptr_set_mark(intptr_t(ptr), mark);
    _ptr.store(x);
  }

  bool compare_and_set(
    Ptr* expectPtr, Ptr* newPtr,
    bool expectMark, bool newMark) {
    intptr_t expect = ptr_ops::ptr_set_mark(intptr_t(expectPtr), expectMark);
    intptr_t newCandidate = ptr_ops::ptr_set_mark(intptr_t(newPtr), newMark);
    return _ptr.compare_exchange_strong(expect, newCandidate);
  }

protected:
  std::atomic<intptr_t> _ptr;
};


ThreadLocalStorage<unsigned int> _seed_tls;

static const size_t MAX_LEVEL = 31;

static std::vector< std::pair< double, size_t > > ComputeRandomTable() {
  static_assert(
    MAX_LEVEL == 31,
    "skip list currently hard-coded for 32 levels");

  // see pg. 335 of Herlihy and Shavit
  std::vector< std::pair< double, size_t > > t;
  double s = 0.75;
  t.push_back( std::make_pair(0.75, 0) );
  for (int i = 1; i <= 30; i++) {
    s += pow(2.0, -(i+2));
    t.push_back( std::make_pair(s, i) );
  }
  // this ensures that we cover the entire probability distribution, regardless
  // of numerical floating point imprecision.
  t.push_back( std::make_pair(std::numeric_limits<double>::max(), 32) );
  return t;
}

static std::vector< std::pair< double, size_t > > t = ComputeRandomTable();

template <typename T>
class concurrent_skip_list {
 private:

  template <typename A, typename B>
  struct PairByFirstCmp {
    bool operator()(const std::pair<A, B>& lhs,
        const std::pair<A, B>& rhs) const {
      return lhs.first < rhs.first;
    }
  };

  static size_t RandomLevel() {
    unsigned int& _seed = _seed_tls.Get();

    // flip coin, and binary search on table
    double flip = double(rand_r(&_seed))/double(RAND_MAX);
    auto it = std::lower_bound(
          t.begin(), t.end(),
          std::make_pair(flip, 0), PairByFirstCmp<double, size_t>());
    assert(it != t.end());
    return it->second;
  }

  struct node {
    enum node_type {
      regular,
      min_value,
      max_value,
    };

    node(node_type type) :
      _next(MAX_LEVEL + 1),
      _topLevel(MAX_LEVEL),
      _type(type) {
      assert(type == min_value || type == max_value);
    }

    node(const T& value, uint32_t height) :
      _value(value),
      _next(height + 1),
      _topLevel(height),
      _type(regular) {
      assert(height <= MAX_LEVEL);
    }


    T _value;
    std::vector< ref_mark_ptr<node> > _next;
    uint32_t _topLevel;
    node_type _type;
  };

  static inline bool less_cmp(const node& a, const T& b) {
    switch (a._type) {
    case node::regular:
      return a._value < b;
    case node::min_value:
      return true;
    case node::max_value:
      return false;
    }
    assert(false);
  }

  static inline bool eq_cmp(const node& a, const T& b) {
    switch (a._type) {
    case node::regular:
      return a._value == b;
    case node::min_value:
    case node::max_value:
      return false;
    }
    assert(false);
  }

  typedef std::vector< node* > node_ptr_vec;

  node* _head;
  node* _tail;

 public:

 public:
  concurrent_skip_list()
    : _head(new node(node::min_value)),
      _tail(new node(node::max_value)) {
    // link _head to _tail
    for (auto &rmp : _head->_next) {
      rmp.set(_tail, false);
    }
    // memory barrier to publish _head and _tail ptr values
    // XXX: do we need this?
    // XXX: what is the C++11 way to do this?
    __sync_synchronize();
  }

  ~concurrent_skip_list()
  {
    // dtor assumes no concurrent writers
    // walk along the bottom row, deleteing elements
    node* cur = _head;
    while (cur) {
      node* next = cur->_next[0].get();
      delete cur;
      cur = next;
    }
  }

  bool add(const T& elem) {
    size_t topLevel = RandomLevel();
    node_ptr_vec preds, succs;
    while (true) {
      rcu::scoped_critical_section crit;
      bool found = find(elem, preds, succs);
      if (found) {
        return false;
      } else {
        node* n(new node(elem, topLevel));
        for (size_t level = 0; level <= topLevel; level++) {
          n->_next[level].set(succs[level], false);
        }
        if (!preds[0]->_next[0].compare_and_set(succs[0], n, false, false)) {
          continue;
        }
        for (size_t level = 1; level <= topLevel; level++) {
          while (true) {
            node* pred = preds[level];
            node* succ = succs[level];
            if (pred->_next[level].compare_and_set(succ, n, false, false)) {
              break;
            }
            find(elem, preds, succs);
          }
        }
        return true;
      }
    }
  }

  bool remove(const T& elem) {
    node_ptr_vec preds, succs;
    node* succ = NULL;
    while (true) {
      rcu::scoped_critical_section crit;
      bool found = find(elem, preds, succs);
      if (!found) {
        return false;
      } else {
        node* nodeToRemove = succs[0];
        for (size_t level = nodeToRemove->_topLevel; level >= 1; level--) {
          bool marked = false;
          succ = nodeToRemove->_next[level].get(marked);
          while (!marked) {
            nodeToRemove->_next[level].compare_and_set(succ, succ, false, true);
            succ = nodeToRemove->_next[level].get(marked);
          }
        }
        bool marked = false;
        succ = nodeToRemove->_next[0].get(marked);
        while (true) {
          bool iMarkedIt = nodeToRemove->_next[0].compare_and_set(succ, succ, false, true);
          succ = succs[0]->_next[0].get(marked);
          if (iMarkedIt) {
            // clean-up optimization
            find(elem, preds, succs);
            return true;
          } else if (marked) {
            return false;
          }
        }
      }
    }
  }

  bool contains(const T& elem) {
    rcu::scoped_critical_section crit;
    bool marked = false;
    node *pred = _head, *curr = NULL, *succ = NULL;
    for (ssize_t level = MAX_LEVEL; level >= 0; level--) {
      curr = pred->_next[level].get();
      while (true) {
        succ = curr->_next[level].get(marked);
        while (marked) {
          curr = pred->_next[level].get();
          succ = curr->_next[level].get(marked);
          //RequestYield(0);
        }
        if (less_cmp(*curr, elem)) {
          pred = curr;
          curr = succ;
        } else {
          break;
        }
      }
    }
    return eq_cmp(*curr, elem);
  }

 private:

  // assumes called from within an RCU critical section
  bool find(const T& elem, node_ptr_vec& preds, node_ptr_vec& succs) {
    assert(rcu::rcu_in_critical_section());
    preds.resize(MAX_LEVEL + 1);
    succs.resize(MAX_LEVEL + 1);
    bool marked = false;
    node *pred = NULL, *curr = NULL, *succ = NULL;
retry:
    while (true) {
      pred = _head;
      for (ssize_t level = MAX_LEVEL; level >= 0; level--) {
        curr = pred->_next[level].get();
        while (true) {
          succ = curr->_next[level].get(marked);
          while (marked) {
            bool snip = pred->_next[level].compare_and_set(curr, succ, false, false);
            if (!snip) goto retry;
            if (level == 0) rcu::rcu_delayed_free(curr);
            curr = succ;
            succ = curr->_next[level].get(marked);
          }
          if (less_cmp(*curr, elem)) {
            pred = curr;
            curr = succ;
          } else {
            break;
          }
        }
        preds[level] = pred;
        succs[level] = curr;
      }
      return eq_cmp(*curr, elem);
    }
  }

};

int NUM_THREADS = 5;

Linearizability linearizability(NUM_THREADS);

class Interface {
 public:
  virtual ~Interface() {}
  virtual int Add(int value) = 0;
  virtual int Remove(int value) = 0;
  virtual int Contains(int value) = 0;
};

class Model : public Interface {
 public:
  Model() : length(0) {}

  virtual int Add(int value) {
    for (int i = 0; i < length; i++) {
      if (values[i] == value) {
        return 0;
      }
    }
    values[length++] = value;
    return 1;
  }

  virtual int Remove(int value) {
    for (int i = 0; i < length; i++) {
      if (values[i] == value) {
        values[i] = values[--length];
        return 1;
      }
    }
    return 0;
  }

  virtual int Contains(int value) {
    for (int i = 0; i < length; i++) {
      if (values[i] == value) {
        return 1;
      }
    }
    return 0;
  }

 private:
  int values[32], length;
};

std::vector<int> workers;

concurrent_skip_list<int>* l;

void ThreadBody(int i) {
  linearizability.ThreadBody(i);

  rcu::rcu_thread_cleanup();
  thread_done[ThreadId()] = true;
}


void Cleaner(int n) {
  for (int i = 0; i < n; i++) {
    bool *ptr = &thread_done[workers[i]];
    RequireResult(true);
    while (*ptr != true) {}
  }

  std::vector<int> tmp;
  std::swap(workers, tmp);

  rcu::rcu_thread_cleanup();
  rcu::rcu_stop_gc_thread();

  delete l;
}

class Implementation : public Interface {
 public:
  Implementation() {
    rcu::rcu_start_gc_thread();
    rcu::_rcu_state_thd_local.Reset();

    _seed_tls.Reset();

    l = new concurrent_skip_list<int>;

    for (int i = 0; i < 64; i++) {
      thread_done[i] = false;
    }

    for (int i = 0; i < NUM_THREADS; i++) {
      workers.push_back(StartThread(&ThreadBody, i));
    }
    StartThread(&Cleaner, NUM_THREADS);
  }

  ~Implementation() {}


  virtual int Add(int value) {
    return l->add(value);
  }

  virtual int Remove(int value) {
    return l->remove(value);
  }

  virtual int Contains(int value) {
    return l->contains(value);
  }

 private:
};

Interface *interface;

struct Configure {
  Configure() {
    linearizability.RegisterImplementation([]() -> void { interface = new Implementation(); }, []() -> void { delete interface; });
    linearizability.RegisterModel([]() -> void { interface = new Model(); }, []() -> void { delete interface; });

    linearizability.AddStep(0, []() -> int { return interface->Add(2); }, "add 2");
    linearizability.AddStep(1, []() -> int { return interface->Add(3); }, "add 3");
    linearizability.AddStep(2, []() -> int { return interface->Remove(2); }, "remove 2");
    linearizability.AddStep(3, []() -> int { return interface->Add(2); }, "add 2");
    //linearizability.AddStep(3, []() -> int { return interface->Remove(2); }, "remove 1");
    //linearizability.AddStep(4, []() -> int { return interface->Contains(4); }, "contains 4?");
    linearizability.AddStep(4, []() -> int { return interface->Contains(2); }, "contains 2");
  }
};

static Configure c;

void Setup() {
  linearizability.Setup();
}

void Finish() {
  linearizability.Finish();
}

