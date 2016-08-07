UNAME := $(shell uname)
SRCS := $(wildcard *.c)
SRCSXX := $(wildcard *.cpp)
OBJS := $(patsubst %.c,%.bc, $(SRCS)) $(patsubst %.cpp,%.bc, $(SRCSXX))
PROGRAMS := nested_txns stack_shared trigger_serial stl privsafety funcattribs
ifneq ($(UNAME), Darwin)
PROGRAMS += intrinsics
endif
PROGRAMSALL := $(patsubst %.bc, %, $(OBJS))
INCLUDE = ../../include
HEADERS = $(wildcard $(INCLUDE)/*.h)
LLVMGCC ?= llvm-gcc
LLVMGXX ?= llvm-g++
ENABLE_AB ?= -fgnu-tm
LLVMBUILD ?= $(shell llvm-config --build-mode)
STMSUPPORTDIR ?= ../../$(LLVMBUILD)/lib/
STMLIBDIR     ?= ../../$(LLVMBUILD)/lib/
TMLINKFLAGS ?= -load=$(STMLIBDIR)/libtanger.so -stmsupport=$(STMSUPPORTDIR) -stmlib=$(STMLIBDIR)
TMLINK ?= ../../$(LLVMBUILD)/bin/tmlink

test: $(PROGRAMS)
all: $(PROGRAMS)
.PHONY: all test clean dis

%.bc: %.c $(HEADERS)
	$(LLVMGCC) -I $(INCLUDE) -emit-llvm -c -o $@ $< $(ENABLE_AB)

%.bc: %.cpp $(HEADERS)
	$(LLVMGXX) -I $(INCLUDE) -emit-llvm -c -o $@ $< $(ENABLE_AB)

intrinsics: intrinsics.bc
	$(TMLINK) $(TMLINKFLAGS) -tanger-resolve-nontxnal=false -o $@ $<
$(filter-out intrinsics,$(PROGRAMS)): %: %.bc
	$(TMLINK) $(TMLINKFLAGS) -o $@ $<

clean:
	rm -f *.bc *.ll tmp.bc* tmp.s* $(PROGRAMS)

install:
	@echo make install not supported.
