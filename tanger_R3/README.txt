Installation
------------

1) Get and install LLVM. You need the llvm-gcc binaries from DTMC and
   you probably have to compile LLVM from source.

2) Configure and build Tanger:
   ./configure --with-llvmsrc=/your/path/to/llvm/sourcedir \
               --with-llvmobj=/your/path/to/llvm/builddir
   make


Using Tanger
------------

Tanger is the backend/transformation part of DTMC. It transforms LLVM
intermediate code (.bc files) so that transactional code uses a TM library.
A frontend such as DTMC's llvm-gcc parses language-level transaction
statements and transforms them into markers and annotations in LLVM IR
that can then be transformed by the Tanger passes. For both language-level
and TM library interfaces, we try to follow the standard specifications.
Older versions of Tanger did not have frontend support for transaction
statements and instead relied on the programmer to use calls to special
functions to demarcate transactional code (tanger_begin() and tanger_commit()).
This is still supported but will not be supported forever.

Tanger can link STM libraries that are either LLVM intermediate
representations archives or native code archives. LLVM IR is of course better
because we can use LLVM's link-time optimizations to, for example, inline
load/store functions. If LLVM cannot compile parts of your STM code, use
native code. See lib/stmdummy for naming conventions and a Makefile.

To compile and transform an application, first compile all C source files to
LLVM IR and then use Tanger's tmlink binary to combine and transform these
files, link a TM library, and finally to generate native code.
There is a Makefile in tests/performance that shows how to do this (see one of
the sub-directory Makefiles for how to use it).
If you want to use a different STM,  build your application using
make STMLIBDIR=/path/to/libtanger-stm.bc/and/libtanger-stm.a
See lib/stmdummy for an example for how to create the required STM libraries.


Directory layout
----------------

doc/
  Additional documentation. Probably outdated.
include/
  Internal STM APIs
lib/stmdummy
  A very simple STM that uses a single lock for concurrency control.
lib/stmsupport
  Support functions for STMs and benchmarks. Not much there yet, and might
  be removed.
lib/tanger
  The LLVM passes that transform applications.
tests/performance
  Performance tests. Useful as starting point or reference for your
  applications.
tests/correctness
  Correctness tests. These test sometimes just check that compilation
  is successful. The tests are not supposed to be meaningful applications.
  If you are looking for examples, rather look at the performance tests.
tests/sandbox
  Small or experimental tests and microbenchmarks.
tests/transformations
  Tests for the transformations performed by the LLVM pass.


Using Tanger with old versions of LLVM
--------------------------------------

We usually make sure that Tanger's code targets the most recent stable release
of LLVM. On demand, we try to make Tanger compatible to older LLVM versions.
This happens automatically in lib/tanger. However, the support for intrinsics
such as llvm.memset.i16 is also affected because the definitions change in
LLVM versions. If you include any of the include/tanger-stm-std-*.h headers
and need compatibility to an older LLVM version, set the TANGER_LLVM_VERSION
macro to major_version + minor_version * 10 when compiling (e.g.,
`llvm-gcc -DTANGER_LLVM_VERSION=204 ...`).


License and copying
-------------------

The license is the GNU General Public License version 2, except for files
that state otherwise. The following files have different licenses and
copyright holders:
tests/performance/rbtree/rbtree.*
This list might not be complete, so check the files.
