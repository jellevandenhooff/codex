#pragma once

#include <functional>
#include <string>

#include "clockvector.h"

extern int StartThread(const std::function<void()>& function);
extern int StartThread(const std::function<void(int)>& function, int arg);
extern int ThreadId();

extern void RequestYield(int);

extern void Found();

extern ClockVector GetClockVector(int thread);
extern void Output(const char* format, ...);

extern void RequireResult(int64_t result);
extern void Annotate(std::string annotation);

