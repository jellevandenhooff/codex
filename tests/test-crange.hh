#pragma once

struct rcu_freed {
  rcu_freed(const char*) {}
};
#define __mpalign__

#include "test-crange-atomic.hh"

using std::atomic;

struct crange;
struct range;

template<class T>
class markptr_ptr;

template<class T>
class markptr_mark;

template<class T>
class markptr {
 protected:
  atomic<uptr> _p;

 public:
  markptr() : _p(0) {}
  markptr(T* v) : _p((uptr) v) {}
  markptr(const markptr<T> &v) : _p(v._p.load()) {}

  void operator=(T* v) { _p = (uptr) v; }
  void operator=(const markptr<T> &v) { _p = v._p.load(); }

  bool operator!=(const markptr<T> &v) const { return _p != v._p; }
  bool operator==(const markptr<T> &v) const { return _p == v._p; }

  markptr_ptr<T>& ptr() {
    return *(markptr_ptr<T>*) this;
  }

  markptr_mark<T>& mark() {
    return *(markptr_mark<T>*) this;
  }

  // Convenience operator to avoid having to write out xx.ptr()->...
  T* operator->() { return ptr(); }

  bool cmpxch(markptr<T> expected, markptr<T> desired) {
    uptr ee = expected._p.load();
    return _p.compare_exchange_weak(ee, desired._p.load());
  }
};

template<class T>
class markptr_ptr : private markptr<T> {
 public:
  void operator=(T *p) {
    uptr p0, p1;
    do {
      p0 = markptr<T>::_p.load();
      p1 = (p0 & 1) | (uptr) p;
    } while (!markptr<T>::_p.compare_exchange_weak(p0, p1));
  }

  T* load() const {
    return (T*) (markptr<T>::_p.load() & ~1);
  }

  operator T*() const { return load(); }
};

template<class T>
class markptr_mark : public markptr<T> {
 public:
  void operator=(bool m) {
    uptr p0, p1;
    do {
      p0 = markptr<T>::_p.load();
      p1 = (p0 & ~1) | !!m;
    } while (!markptr<T>::_p.compare_exchange_weak(p0, p1));
  }

  bool load() const {
    return markptr<T>::_p.load() & 1;
  }

  operator bool() const { return load(); }
};

struct range : public rcu_freed {
 public:
  u64 key;
  u64 size;
  void *value;
  atomic<int> curlevel;  // the current levels it appears on
  int nlevel;            // the number of levels this range should appear
  crange *cr;            // the crange this range is part of
  markptr<range>* next;  // one next pointer per level
  /*spinlock *lock;        // on separate cache line?*/ Mutex lock;

  range(crange *cr, u64 k, u64 sz, void *v, range *n, int nlevel = 0);
  ~range();
  virtual void do_gc() {
    delete this;
  }

  void print(int l);
  void dec_ref(void);
  int lockif(markptr<range> e);
} __mpalign__;

class range_iterator {
 private:
  range *_e;

 public:
  range_iterator(range *e) : _e(e) {}
  range_iterator& operator++() { _e = _e->next[0].ptr(); return *this; }
  range*& operator*() { return _e; }
  bool operator==(const range_iterator &other) { return _e == other._e; }
  bool operator!=(const range_iterator &other) { return _e != other._e; }
};

struct crange {
 private:
  range *crange_head;    // a crange skip list starts with a sentinel range (key 0, sz 0)

 public:
  int nlevel;                  // number of levels in the crange skip list
  crange(int nlevel);
  ~crange(void);
  void del(u64 k, u64 sz);
  void add(u64 k, u64 sz, void *v);
  range* search(u64 k, u64 sz, int mod = 0);
  void print(int);
  void check(struct range *absent);
  int del_index(range *p0, range **e, int l);
  void add_index(int l, range *e, range *p1, markptr<range> s1);
  int lock_range(u64 k, u64 sz, int l, range **er, range **pr, range **fr, range **lr, range **sr);
  int find_and_lock(u64 k, u64 sz, range **p0, range **f0, range **l0, range **s0);

  range_iterator begin() const { return range_iterator(crange_head->next[0].ptr()); };
  range_iterator end() const { return range_iterator(0); };
};

static inline range_iterator
begin(const crange &cr)
{
  return cr.begin();
}

static inline range_iterator
end(const crange &cr)
{
  return cr.end();
}
