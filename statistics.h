#pragma once

#include <map>
#include <string>
#include <sstream>

class StatisticHolder {
 public:
  virtual std::string Dump() const = 0;
  virtual bool ShouldDump() const = 0;
  virtual void Reset() = 0;
};

template<class T>
class StatisticHolderImpl : public StatisticHolder {
 public:
  StatisticHolderImpl(T initial, bool output_initial) : 
      initial_(initial), value_(initial), output_initial_(output_initial) {}
  T* pointer_to_value() {
    return &value_;
  }
  virtual std::string Dump() const {
    std::stringstream ss;
    ss << value_;
    return ss.str();
  }
  virtual bool ShouldDump() const {
    return output_initial_ || initial_ != value_;
  }
  virtual void Reset() {
    value_ = initial_;
  }
 private:
  T initial_;
  T value_;
  bool output_initial_;
};

extern std::map<std::string, StatisticHolder*>* statistics;

inline void EnsureStatistics() {
  if (statistics == nullptr) {
    statistics = new std::map<std::string, StatisticHolder*>();
  }
}

template<class T>
T& RegisterStatistic(std::string name, T value=T(), bool output_initial=false) {
  auto statistic = new StatisticHolderImpl<T>(value, output_initial);
  EnsureStatistics();
  (*statistics)[name] = statistic;
  return *statistic->pointer_to_value();
}

extern void DumpStatisticsToStderr();

