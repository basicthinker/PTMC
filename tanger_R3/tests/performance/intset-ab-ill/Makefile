# Defines the executables to be built
APPLICATIONS = intset_ll intset_rb intset_sl
# Defines which objects belong to each application 
intset_ll_OBJS = intset_ll.bc
intset_rb_OBJS = intset_rb.bc
intset_sl_OBJS = intset_sl.bc

# GCC thread-local storage (enable if supported on target architecture)
ifdef NOTLS
 DEFINES += -UTLS
else
 DEFINES += -DTLS
endif

include ../Makefile.common
