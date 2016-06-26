# The following settings are required for Gibraltar.
# The user hast to set GIBRALTARLIBDIR to the directory that contains the
# Gibraltar libary.
GIBRALTARLIBDIR ?= .
NATIVELIBSPRE = $(GIBRALTARLIBDIR)/lib-gibraltar-pinapp.a
#TANGERPASSOPTS  ?= -tanger-gibraltaronly
TANGERPASSOPTS  ?= -tanger-gibraltar
#TANGERPASSOPTS  += -tanger-gibraltar-stackhints=false

# executables to be built
APPLICATIONS = calls-external-lib stackhint

# Use common makefile rules
MAKEFILEREL = .
include ../performance/Makefile.common

# Defines which objects belong to the applications and links them
calls-external-lib-tangerbase.bc: calls-external-lib.bc
	$(LLVMLD) -link-as-library -disable-inlining -o $@ $^

stackhint-tangerbase.bc: stackhint.bc
	$(LLVMLD) -link-as-library -disable-inlining -o $@ $^
