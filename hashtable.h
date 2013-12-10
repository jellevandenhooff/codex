#pragma once

const int kLogHashSize = 13;
const int kHashSize = 1 << kLogHashSize;
const int kHashMask = kHashSize - 1;

template<class T>
struct HashElement {
  HashElement() {}
  intptr_t address;
  T value;
  int epoch;
};

template<class T>
class HashTable {
 public:
  HashTable() {
    for (int i = 0; i < kHashSize; i++) {
      elements_[i].epoch = 0;
    }
    epoch = 1;
  }

  T& operator[](intptr_t address) {
    int key = address & kHashMask;
    while (true) {
      HashElement<T>& element = elements_[key];
      if (element.epoch == epoch) {
        if (element.address == address) {
          return element.value;
        } else {
          key = (key + 1) & kHashMask;
        }
      } else {
        element.address = address;
        element.value.Reset();
        element.epoch = epoch;
        return element.value;
      }
    }
  }

  void Reset() {
    epoch++;
  }

 private:
  HashElement<T> elements_[kHashSize];
  int epoch;
};

