#pragma once

#include <cassert>

#include <memory>
#include <string>
#include <vector>

enum class TransitionType : int {
  NONE = 0,
  WRITE = 1,
  READ = 2,
  CAS = 3,
  READ_GE = 4,
  ATOMICRMW = 5,
};

struct Result {
  int64_t returned_value;
  bool does_write;
  int64_t written_value;

  Result(int64_t returned_value, int64_t written_value) :
    returned_value(returned_value), does_write(true),
    written_value(written_value) {}
  Result(int64_t returned_value) :
    returned_value(returned_value), does_write(false) {}
};

class Transition {
 public:
  Transition() {}

  Transition(TransitionType type, int8_t *address, int32_t length, 
      int8_t* file, bool is_atomic) :
          type_(type), address_(address), has_required_(false), required_(0),
          length_(length), arg0_(0), arg1_(0), is_atomic_(is_atomic),
          file_(file) {}
  Transition(TransitionType type, int8_t *address, int32_t length,
      int64_t arg, int8_t* file, bool is_atomic) :
          type_(type), address_(address), has_required_(false), required_(0),
          length_(length), arg0_(arg), arg1_(0), is_atomic_(is_atomic),
          file_(file) {}
  Transition(TransitionType type, int8_t *address, int32_t length,
      int64_t arg0, int64_t arg1, int8_t* file, bool is_atomic) :
          type_(type), address_(address), has_required_(false), required_(0),
          length_(length), arg0_(arg0), arg1_(arg1), is_atomic_(is_atomic),
          file_(file) {}

  Result DetermineResult(int64_t value) const;
  std::string Format(int64_t value) const;
  std::string Dump(int thread, int step, int64_t value) const;

  inline bool ConflictsWith(const Transition& o) const {
    if (address_ != o.address_) {
      return false;
    } else if (can_write() && o.can_write()) {
      return true;
    } else if (!can_write() && !o.can_write()) {
      return false;
    } else {
      return true;
    }
  }

  bool DetermineRunnable(int64_t value) const {
    if (has_required_) {
      return DetermineResult(value).returned_value == required_;
    } else {
      return true;
    }
  }

  inline bool DetermineRunnable() const {
    return !has_required_ || DetermineRunnable(Read());
  }

  inline int64_t Read() const {
    switch (length_) {
      case 1:
        return (uint8_t)(*reinterpret_cast<int8_t*>(address_));
      case 2:
        return (uint16_t)(*reinterpret_cast<int16_t*>(address_));
      case 4:
        return (uint32_t)(*reinterpret_cast<int32_t*>(address_));
      case 8:
        return *reinterpret_cast<int64_t*>(address_);
      default:
        assert(0);
    }
  }

  inline void Write(int64_t value) const {
    switch (length_) {
      case 1:
        *reinterpret_cast<int8_t*>(address_) = value;
        break;
      case 2:
        *reinterpret_cast<int16_t*>(address_) = value;
        break;
      case 4:
        *reinterpret_cast<int32_t*>(address_) = value;
        break;
      case 8:
        *reinterpret_cast<int64_t*>(address_) = value;
        break;
      default:
        assert(0);
    }
  }

  inline bool can_write() const {
    return type_ != TransitionType::READ && type_ != TransitionType::READ_GE;
  }
  inline bool has_required() const {
    return has_required_;
  }
  inline int64_t required_result() const {
    return required_;
  }
  inline void set_required(int64_t required) {
    has_required_ = true;
    required_ = required;
  }
  inline TransitionType type() const {
    return type_;
  }
  inline int8_t* address() const {
    return address_;
  }
  inline int32_t length() const {
    return length_;
  }
  inline bool is_atomic() const {
    return is_atomic_;
  }
  inline std::shared_ptr<std::vector<std::string>> annotations() const {
    return annotations_;
  }
  inline void set_annotations(
      std::shared_ptr<std::vector<std::string>> annotations) {
    annotations_ = annotations;
  }

 private:
  int8_t *address_;
  bool has_required_;
  int64_t required_;
  int64_t arg0_, arg1_;
  int32_t length_;
  TransitionType type_;
  bool is_atomic_;
  int8_t* file_;
  std::shared_ptr<std::vector<std::string>> annotations_;
};

