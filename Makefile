O	 := obj
CLANGPP	 := clang++
OPT	 := opt
TEST_CC	 := $(wildcard tests/test-simple*.cc)
TEST_BIN := $(patsubst tests/%.cc,$(O)/%,$(TEST_CC))

CXXFLAGS      := -std=c++11 -g -O2 -Iold-boost.lockfree -Ihacked-cds-1.3.1 -I.
LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
LIBS := -lboost_context -lcityhash

ifeq ($(shell uname),Darwin)
CXXFLAGS      := $(CXXFLAGS) -stdlib=libc++ -Wno-lambda-extensions
LLVM_LDFLAGS  := -Wl,-flat_namespace -Wl,-undefined,suppress
LIBS := -lboost_context-mt -lcityhash
endif

ifeq ($(shell hostname),ben)
CXXFLAGS := $(CXXFLAGS) -I/home/am3/jelle/include
LIBS := -L/home/am3/jelle/lib -lboost_context -lcityhash
endif

CODEX_CC := hbhistory.cc hhbhistory.cc interceptor.cc interface.cc \
  linearizability.cc main.cc pinner.cc scheduler.cc statistics.cc \
  trace_builder.cc transition.cc
CODEX_LL := $(patsubst %.cc,$(O)/%.ll,$(CODEX_CC))

.PHONY: all
all:	$(TEST_BIN)

DEPS := $(wildcard obj/*.d)
-include $(DEPS)

$(O)/%.ll: %.cc
	@mkdir -p $(@D)
	$(CLANGPP) $< -MD -S -o $@ $(CXXFLAGS) -emit-llvm

$(O)/llvm_mod/%.o: llvm_mod/%.cc
	@mkdir -p $(@D)
	$(CLANGPP) $< -c -o $@ $(LLVM_CXXFLAGS) -fPIC

$(O)/llvm_mod/%.so: $(O)/llvm_mod/%.o
	$(CLANGPP) $< -o $@ $(LLVM_LDFLAGS) -shared -fPIC

$(O)/%-intercepted.ll: $(O)/%.ll $(O)/llvm_mod/pass.so
	$(OPT) -load=$(O)/llvm_mod/pass.so -intercept $< -S -o $@

$(O)/test-cds: $(patsubst %.cc,$(O)/%-intercepted.ll,$(wildcard cds/*.cc)) $(CODEX_LL)
	$(CLANGPP) $^ -o $@ $(CXXFLAGS) $(LIBS)

$(O)/%: $(O)/tests/%-intercepted.ll $(CODEX_LL)
	$(CLANGPP) $^ -o $@ $(CXXFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -rf $(O)

.PRECIOUS: $(O)/%.bc $(O)/%.ll $(O)/%-intercepted.ll
.PRECIOUS: $(O)/llvm_mod/%.o $(O)/llvm_mod/%.so
