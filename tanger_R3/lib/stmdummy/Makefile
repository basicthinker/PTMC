# We build two things:
#
# tanger-stm.bc will be linked via LLVM to the application, so
#   performance-critical functions should go there.
#
# tanger-stm.a will be linked as binary to the application, and can contain
#   support code and anything else that LLVM is not able to compile
#   (or should not).

LEVEL=../..

# Modify this for other STMs that exist outside of the Tanger source tree
TANGERINC ?= -I $(PROJ_SRC_ROOT)/include

CFLAGS += -Wall -O3 $(TANGERINC) -std=c99
SRCDIR = $(PROJ_SRC_ROOT)/lib/stmdummy

# we could use LLVM's build system, but we can't create different libraries
# out of one directory. Thus, we build the .a separately
SOURCES=dummystm-ls.c dummystm.c
MODULE_NAME=libtanger-stm
LIBRARYNAME=tanger-stm
#BUILD_ARCHIVE=1
# TODO: we should build a bytecode library, so that we can internalize
# private functions
#BYTECODE_LIBRARY=1

# Don't install DummySTM
NO_INSTALL=1

include $(LEVEL)/Makefile.common

# code for a library, copied from LLVM's Makefile.rules
# see the LLVM license
all-local:: $(LibName.A)

#$(LibName.A): $(ObjDir)/dummystm.o $(LibDir)/.dir
#	$(Echo) Building $(BuildMode) Archive Library $(notdir $@)
#	-$(Verb) $(RM) -f $@
#	$(Verb) $(Archive) $@ $(ObjDir)/dummystm.o
#	$(Verb) $(Ranlib) $@

clean-local::
ifneq ($(strip $(LibName.A)),)
	-$(Verb) $(RM) -f $(LibName.A)
endif

ifdef NO_INSTALL
install-local::
	$(Echo) Install circumvented with NO_INSTALL
uninstall-local::
	$(Echo) Uninstall circumvented with NO_INSTALL
else
DestArchiveLib := $(PROJ_libdir)/lib$(LIBRARYNAME).a

install-local:: $(DestArchiveLib)

$(DestArchiveLib): $(LibName.A) $(PROJ_libdir)
	$(Echo) Installing $(BuildMode) Archive Library $(DestArchiveLib)
	$(Verb) $(MKDIR) $(PROJ_libdir)
	$(Verb) $(LTInstall) $(LibName.A) $(DestArchiveLib)

uninstall-local::
	$(Echo) Uninstalling $(BuildMode) Archive Library $(DestArchiveLib)
	-$(Verb) $(RM) -f $(DestArchiveLib)
endif

#$(LibName.O): $(ObjectsO) $(LibDir)/.dir
#		$(Echo) Linking $(BuildMode) Object Library $(notdir $@)
#		$(Verb) ld -melf_i386 -shared -o $@ $(ObjectsO)
