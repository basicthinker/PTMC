CC := gcc
LPTHREAD := -lpthread
TANGERPASS  ?= ../../Debug/lib/
STMLIBDIR  ?= ../../Debug/lib/
STMSUPPORTDIR  ?= ../../Debug/lib/
TMLINK  ?= ../../Debug/bin/tmlink
TANGERINCLUDE ?= ../../include
BINS = tls-stack-pthread stack-rollback
CFLAGS = -Wall -O2 -fgnu-tm

.PHONY: all dis clean

all: $(BINS)

tls-stack-pthread: tls-stack-pthread.c
	$(CC) -o $@ -O3 $(LPTHREAD) $<

stack-rollback: stack-rollback.c
	llvm-gcc $(CFLAGS) -emit-llvm -c -o stack-rollback-pre.bc stack-rollback.c -Wall
	llvm-ld stack-rollback-pre.bc -link-as-library -o stack-rollback.bc
	$(TMLINK) stack-rollback.bc -o stack-rollback -tanger=$(TANGERPASS) -stmlib=$(STMLIBDIR) -stmsupport=$(STMSUPPORTDIR)

nontxnal-func-tags: nontxnal-func-tags.c
	llvm-gcc $(CFLAGS) -emit-llvm -c -o nontxnal-func-tags.bc $< -Wall -I $(TANGERINCLUDE)
	opt -load $(TANGERPASS) nontxnal-func-tags.bc -tanger -o nontxnal-func-tags-tanger.bc -f
	llvm-ld nontxnal-func-tags-tanger.bc -link-as-library -o nontxnal-func-tags-tanger-l.bc ../../lib/stmdummy/tanger-stm.bc
	llc nontxnal-func-tags-l.bc -f -o nontxnal-func-tags-tanger.s
	rm nontxnal-func-tags-l.bc
	gcc -o $@ $(LPTHREAD) nontxnal-func-tags-tanger.s

dis: all $(patsubst %.bc, %.ll, $(wildcard *.bc))
%.ll: %.bc
	llvm-dis $< -f -o $@

clean:
	rm -f *.bc *.o *.s *.ll $(BINS)
	
