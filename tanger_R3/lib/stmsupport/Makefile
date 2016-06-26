# We build two things:
#
# tanger-stm.bc will be linked via LLVM to the application, so
#   performance-critical functions should go there.
#
# tanger-stm.a will be linked as binary to the application, and can contain
#   support code and anything else that LLVM is not able to compile.

LEVEL=../..

#TANGERINC ?= -I ../../include
CFLAGS += -Wall -O3 -fno-exceptions -std=c99

MODULE_NAME=stmsupport

include $(LEVEL)/Makefile.common
