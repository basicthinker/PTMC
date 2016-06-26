##===- tools/llvmc/Makefile --------------------------------*- Makefile -*-===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open
# Source License. See LICENSE.TXT for details.
#
##===----------------------------------------------------------------------===##

LEVEL = ../..

BUILT_SOURCES = TMLink.inc
USEDLIBS := tanger.a

ExtraLibs += $(TMLINKLDFLAGS)

# Everything in Makefile.rules that is enabled by LLVMC_BASED_DRIVER:
# (continued below after including Makefile.common)  
#TOOLNAME = $(LLVMC_BASED_DRIVER)
TOOLNAME = tmlink
LLVMLIBS = CompilerDriver.a
LINK_COMPONENTS = support system bitreader scalaropts bitwriter ipo
TABLEGEN_INC_FILES_COMMON = 1

include $(LEVEL)/Makefile.common

# More stuff from Makefile.rules for LLVMC-based drivers.
# Included after Makefile.common so that all definitions from there are available.
TDSrc := $(sort $(strip $(wildcard $(PROJ_SRC_DIR)/*.td)) \
                $(strip $(wildcard $(PROJ_OBJ_DIR)/*.td)))

TDCommon := $(strip $(wildcard \
                $(LLVM_SRC_ROOT)/include/llvm/CompilerDriver/*.td))

TDFiles := $(TDSrc) $(TDCommon)

$(INCTMPFiles) : $(TBLGEN) $(TDFiles)

$(ObjDir)/%.inc.tmp: %.td $(ObjDir)/.dir
	$(Echo) "Building LLVMC compilation graph description with tblgen"
	$(Verb) $(TableGen) -gen-llvmc -o $(call SYSPATH, $@) $<

clean-local::
	-$(Verb) $(RM) -f $(INCFiles)
