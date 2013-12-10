#include "transition.h"

#include <cassert>

#include <string>
#include <sstream>
#include <vector>

Result Transition::DetermineResult(int64_t value) const {
  switch (type_) {
  case TransitionType::READ:
    return Result(value);
  case TransitionType::WRITE:
    return Result(0, arg0_);
  case TransitionType::CAS:
    if (value == arg0_) {
      return Result(value, arg1_);
    } else {
      return Result(value);
    }
  case TransitionType::READ_GE:
    return Result(value >= arg0_);
  case TransitionType::ATOMICRMW: {
    switch (arg0_) {
    case 0:
      return Result(value, arg1_);
    case 1:
      return Result(value, value + arg1_);
    case 2:
      return Result(value, value - arg1_);
    }
  }
  default:
    assert(0);
  }
}

std::string Transition::Format(int64_t value) const {
  std::stringstream ss;

  switch (type_) {
  case TransitionType::READ:
    ss << "Read *" << (void*)address_ << " = " << (void*)value;
    break;
  case TransitionType::WRITE:
    ss << "Write *" << (void*)address_ << " = " << (void*)arg0_;
    break;
  case TransitionType::CAS:
    if (value == arg0_) {
      ss << "CAS success *" << (void*)address_ << " from " << (void*)arg0_ <<
          " to " << (void*)arg1_;
    } else {
      ss << "CAS fail *" << (void*)address_ << " from " << (void*)arg0_ << 
          " to " << (void*)arg1_ << "; was " << (void*)value;
    }
    break;
  case TransitionType::READ_GE:
    ss << "Compared *" << (void*)address_ << " = " << (void*)value << " to " <<
        (void*)arg0_;
    break;
  case TransitionType::ATOMICRMW: {
    switch (arg0_) {
    case 0:
      ss << "Exchanged *" << (void*)address_ << " = " << (void*)value << 
          " with " << (void*)arg0_;
      break;
    case 1:
      ss << "*" << (void*)address_ << " = " << (void*)value << " += " <<
        (void*)arg0_;
      break;
    case 2:
      ss << "*" << (void*)address_ << " = " << (void*)value << " -= " <<
        (void*)arg0_;
      break;
    }
    break;
  }
  default:
    assert(0);
  }
  ss << " (" << length() << " bytes)";
  return ss.str();
}

std::string Transition::Dump(int thread, int step, int64_t value) const {
  std::stringstream ss;

  Result res = DetermineResult(value);

  ss << "{'does_write': " << (res.does_write ? "True": "False") << ", ";
  ss << "'address': '" << (void*)address_ << "', ";
  ss << "'type': 'transition', ";
  ss << "'value': '" << (void*)value << "', ";
  ss << "'thread': " << thread << ", ";
  ss << "'step': " << step << ", ";
  if (res.does_write) {
    ss << "'new_value': '" << (void*)res.written_value << "', ";
  }
  ss << "'length': " << length_ << ", ";
  ss << "'description': '";

  switch (type_) {
  case TransitionType::READ:
    ss << "Read " << (void*)address_ << " = " << (void*)value;
    break;
  case TransitionType::WRITE:
    ss << "Write " << (void*)address_ << " = " << (void*)arg0_;
    break;
  case TransitionType::CAS:
    if (value == arg0_) {
      ss << "CAS " << (void*)address_ << " from " << (void*)arg0_ << " to " <<
          (void*)arg1_;
    } else {
      ss << "CAS fail " << (void*)address_ << " from " << (void*)arg0_ << 
          " to " << (void*)arg1_ << "; was " << (void*)value;
    }
    break;
  case TransitionType::READ_GE:
    ss << "Compared " << (void*)address_ << " = " << (void*)value << " to " <<
        (void*)arg0_;
    break;
  case TransitionType::ATOMICRMW: {
    switch (arg0_) {
    case 0:
      ss << "Exchanged " << (void*)address_ << " = " << (void*)value << 
          " with " << (void*)arg0_;
      break;
    case 1:
      ss << (void*)address_ << " = " << (void*)value << " += " << (void*)arg0_;
      break;
    case 2:
      ss << (void*)address_ << " = " << (void*)value << " -= " << (void*)arg0_;
      break;
    }
    break;
  }
  default:
    assert(0);
  }
  ss << "'";
  if (file_ != nullptr) {
    ss << ", 'trace': " << file_;
  }
  ss << "}";
  return ss.str();
}

