#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=tanger
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1
SHARED_LIBRARY=1

KEEP_SYMBOLS=1

CXXFLAGS += -DLLVMVersion$(subst 2.,2,$(LLVMVersion))

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

# gather buildinfo if reporting is enabled
$(eval $(shell egrep ^ENABLE_REPORTING= $(LEVEL)/Makefile.common))
ifeq ($(ENABLE_REPORTING),1)
  SVN_URL := $(shell svn info $(LEVEL) | perl -ne '/URL: (.+)/ and print $$1;')
  SVN_MAXREV := $(shell svn info -R $(LEVEL) | perl -ne '/Revision: (.+)/ and print "$$1\n";' | sort -n | tail -1)
  SVN_CHANGED := $(if $(shell svn st -q $(LEVEL)),1,0)
ifneq ($(DSASRCDIR),)
  DSA_SVN_URL := $(shell svn info $(DSASRCDIR) | perl -ne '/URL: (.+)/ and print $$1;')
  DSA_SVN_MAXREV := $(shell svn info -R $(DSASRCDIR) | perl -ne '/Revision: (.+)/ and print "$$1\n";' | sort -n | tail -1)
  DSA_SVN_CHANGED := $(if $(shell svn st -q $(DSASRCDIR)),1,0)
endif
endif

.PHONY: buildinfo.new

transformations.cpp: buildinfo.inc

buildinfo.inc: buildinfo.new
	@echo '#define BUILDINFO_SVN_URL "$(SVN_URL)"' > buildinfo.new
	@echo '#define BUILDINFO_SVN_MAXREV $(SVN_MAXREV)' >> buildinfo.new
	@echo '#define BUILDINFO_SVN_CHANGED $(SVN_CHANGED)' >> buildinfo.new
ifneq ($(DSASRCDIR),)
	@echo '#define BUILDINFO_DSA_SVN_URL "$(DSA_SVN_URL)"' >> buildinfo.new
	@echo '#define BUILDINFO_DSA_SVN_MAXREV $(DSA_SVN_MAXREV)' >> buildinfo.new
	@echo '#define BUILDINFO_DSA_SVN_CHANGED $(DSA_SVN_CHANGED)' >> buildinfo.new
endif
	@diff buildinfo.new buildinfo.inc -q || cp buildinfo.new buildinfo.inc
	@rm buildinfo.new

# TODO clean up
#clean:
#	rm buildinfo.inc