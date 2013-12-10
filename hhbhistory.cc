#include "hhbhistory.h"

#include <iomanip>
#include <sstream>

#include "config.h"

struct __attribute__ ((__packed__)) NodeHashBuffer {
  int thread;
  Hash hashes[kMaxThreads];
};

void HHBHistory::AddTransition(int thread, const Transition& transition) {
  HBHistory::AddTransition(thread, transition);

  NodeHashBuffer buffer;
  buffer.thread = thread;
  for (int other_thread = 0; other_thread < kMaxThreads; other_thread++) {
    if (other_thread == thread) {
      continue;
    }

    int time = current_cv_for(thread)[other_thread];

    if (time >= 0) {
      buffer.hashes[other_thread] = hash_at_[time];
    } else {
      buffer.hashes[other_thread] = 0;
    }
  }

  buffer.hashes[thread] = current_hash_for_[thread];

  Hash hash = CityHash64((char*)&buffer, sizeof(buffer)); 
  current_hash_for_[thread] = hash;
  hash_at_.push_back(hash);
}

struct __attribute__ ((__packed__)) CombineHashBuffer {
  Hash hashes[kMaxThreads];
};

Hash HHBHistory::CombineCurrentHashes() const {
  CombineHashBuffer buffer;
  for (int thread = 0; thread < kMaxThreads; thread++) {
    buffer.hashes[thread] = current_hash_for_[thread];
  }
  return CityHash64((char*)&buffer, sizeof(buffer)); 
}

Hash HHBHistory::CombineCurrentHashesWithLast() const {
  NodeHashBuffer buffer;
  for (int thread = 0; thread < kMaxThreads; thread++) {
    buffer.hashes[thread] = current_hash_for_[thread];
  }
  if (length() > 0) {
    buffer.thread = thread_at(length() - 1);
  } else {
    buffer.thread = -1;
  }
  return CityHash64((char*)&buffer, sizeof(buffer)); 
}

void HHBHistory::Reset() {
  HBHistory::Reset();
  current_hash_for_.clear();
  hash_at_.clear();
  for (int i = 0; i < kMaxThreads; i++) {
    current_hash_for_[i] = 0;
  }
}

std::string ConvertHashToString(Hash hash) {
  std::stringstream ss;
  ss << std::setw(16) << std::setfill('0') << std::hex;
  ss << hash; // hash.first << hash.second;
  // FIXME: Don't always want to chop hashes!
  return ss.str().substr(0, 8);
}

