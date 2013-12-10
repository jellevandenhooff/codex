#pragma once

#include <vector>

#include <city.h>

#include "hbhistory.h"
#include "threadset.h"
#include "transition.h"

typedef uint64 Hash;

class HHBHistory : public HBHistory {
 public:
  virtual void AddTransition(int thread, const Transition& transition);
  virtual void Reset();

  Hash CombineCurrentHashes() const;
  Hash CombineCurrentHashesWithLast() const;

  inline Hash hash_at(int time) const {
    return hash_at_[time];
  }
  
  inline Hash current_hash_for(int thread) const {
    return current_hash_for_[thread];
  }

 private:
  ThreadMap<Hash> current_hash_for_;
  std::vector<Hash> hash_at_;  
};

std::string ConvertHashToString(Hash hash);
