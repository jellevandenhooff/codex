#include "codex_interface.h"
#include "program_interface.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>

#include "config.h"
#include "hhbhistory.h"
#include "interceptor.h"
#include "predictable_alloc.h"
#include "threadmap.h"
#include "transition.h"

static PredictableAlloc* predictable_alloc = nullptr;

static inline PredictableAlloc* GetPredictableAlloc() {
  if (predictable_alloc == nullptr) {
    predictable_alloc = new PredictableAlloc();
  }
  return predictable_alloc;
}

extern void Setup();
extern void Finish();

static Interceptor* interceptor = nullptr;

Interceptor* SetupInterfaceAndInterceptor() {
  assert(interceptor == nullptr);

  GetPredictableAlloc()->StoreOffsetAsBase();
  interceptor = new Interceptor([]() { 
    GetPredictableAlloc()->ResetOffsetToBase();
    Setup();
  }, Finish);

  return interceptor;
}

// Methods exposed to user code to interact with the runtime environment, such
// as starting new threads or learning a current clockvector.

int StartThread(const std::function<void()>& task) {
  return interceptor->StartThread(task);
}

int StartThread(const std::function<void(int)>& task, int arg) {
  return interceptor->StartThread(std::bind(task, arg));
}

int ThreadId() {
  return interceptor->current_thread();
}

void RequestYield(int) {
}

void Found() {
  interceptor->FoundBug();
}

ClockVector GetClockVector(int thread) {
  return interceptor->history()->current_cv_for(thread);
}

void Output(const char* format, ...) {
  if (show_program_output) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
  }
}

// Methods exposed to user code to provide extra information on upcoming
// transitions, such as a required result or an annotation.

struct NextTransitionInfo {
  NextTransitionInfo() {}

  bool has_required;
  int64_t required;
  std::vector<std::string> annotations;
};

static ThreadMap<NextTransitionInfo> next_transition_info;

void RequireResult(int64_t result) {
  auto& info = next_transition_info[interceptor->current_thread()];
  info.has_required = true;
  info.required = result;
}

void Annotate(std::string annotation) {
  auto& info = next_transition_info[interceptor->current_thread()];
  info.annotations.push_back(annotation);
}

// Handlers for intercepted memory accesses. All such accesses first get
// intercepted below, and then get passed to the interceptor.

static int64_t Intercept(Transition transition) {
  // Intercepted code can have static initializaton code that that runs before
  // any of Codex. All such code runs transparently.
  if (interceptor != nullptr) {
    int thread = interceptor->current_thread();

    // Intercepted code can have setup code that we do not attempt to
    // interleave, and also run transparently.
    if (thread != Scheduler::kOriginalThread) {
      // Store extra information in the transition object that was passed
      // out-of-band through Annotate and RequireResult.
      auto& info = next_transition_info[thread];
      if (info.has_required) {
        transition.set_required(info.required);
        info.has_required = false;
      }
      if (!info.annotations.empty()) {
        auto new_annotations = std::make_shared<std::vector<std::string>>();
        std::swap(*new_annotations, info.annotations);
        transition.set_annotations(new_annotations);
      }

      interceptor->ReachedTransition(transition);

      if (show_all_transitions) {
        fprintf(stderr, "% 3d [% 2d]: %s\n", 
            interceptor->history()->length() - 1,
            interceptor->current_thread(),
            transition.Format(transition.Read()).c_str());
      }
    }
  }

  // Execute the transition.
  Result result = transition.DetermineResult(transition.Read());
  if (result.does_write) {
    transition.Write(result.written_value);
  }
  return result.returned_value;
}

extern "C"
int8_t* InterceptNew(int64_t size) {
  return GetPredictableAlloc()->Alloc(size);
}

extern "C"
void InterceptDelete(int8_t* ptr) {
}

extern "C"
void InterceptStore(int8_t* address, int64_t value, int32_t length, 
    int32_t is_atomic, int8_t* file) {
  Intercept(Transition(TransitionType::WRITE, address, length, value, file,
        is_atomic));
}

extern "C"
int64_t InterceptLoad(int8_t* address, int32_t length, int32_t is_atomic,
    int8_t* file) {
  return Intercept(Transition(TransitionType::READ, address, length, file,
        is_atomic));
}

extern "C"
int64_t InterceptCmpXChg(int8_t* address, int64_t expected, 
    int64_t replacement, int32_t length, int8_t* file) {
  return Intercept(Transition(TransitionType::CAS, address, length, expected,
        replacement, file, true));
}

extern "C"
int64_t InterceptAtomicRMW(int8_t* address, int64_t value, int32_t type, 
    int32_t length, int8_t* file) {
  return Intercept(Transition(TransitionType::ATOMICRMW, address, length, type,
        value, file, true));
}

extern "C"
void InterceptMemset(int8_t* dest, int8_t val, int32_t len, int32_t align,
    bool is_volatile) {
  memset(dest, val, len);
}

extern "C"
void InterceptMemcpy(int8_t* dest, int8_t* src, int32_t len, int32_t align,
    bool is_volatile) {
  memcpy(dest, src, len);
}

extern "C"
void InterceptFence() {
}

