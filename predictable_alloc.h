#pragma once

#include <cstdio>
#include <cstring>

class PredictableAlloc {
 public:
  PredictableAlloc() {
    buffer_ = base_ = offset_ = new int8_t[1024 * 1024 * 64];
  }

  ~PredictableAlloc() {
    delete buffer_;
  }

  int8_t* Alloc(int64_t size) {
    size += (8 - (size % 8)) % 8;

    int8_t* slab = offset_;
    offset_ += size;

    memset(slab, 0, size);
    return slab;
  }

  void StoreOffsetAsBase() {
    base_ = offset_;
  }

  void ResetOffsetToBase() {
    offset_ = base_;
  }

 private:
  int8_t *buffer_, *base_, *offset_;
};

