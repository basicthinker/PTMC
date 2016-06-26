dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([Tanger],[0.80],[bugs@tinystm.org])

dnl Identify where LLVM source tree is
LLVM_SRC_ROOT="../../"
LLVM_OBJ_ROOT="../../"

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)

dnl Tell autoconf that the auxilliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR($LLVM_SRC/autoconf)

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.common.in"])

dnl --enable-dsa
AC_ARG_ENABLE(dsa, AS_HELP_STRING(
 --enable-dsa,[Enable transformations that use DSA (default is NO)]),,enableval="no")
if test ${enableval} = "yes" ; then
  AC_SUBST(ENABLE_DSA,[[ENABLE_DSA=1]])
  AC_DEFINE(ENABLE_DSA, [1], [Defined if DSA is available])
fi

dnl Set the DSA obj directory.
AC_ARG_WITH(dsaobjdir,
  AS_HELP_STRING([--with-dsaobjdir],
    [Specify location of DSA object directory]),
  AC_DEFINE(ENABLE_DSA, [1], [Defined if DSA is available]), )
case "$withval" in
  /* | [[A-Za-z]]:[[\\/]]*)  ;;
  *) AC_MSG_ERROR([Invalid path for --with-dsaobjdir. Provide full path]) ;;
esac
AC_DEFINE_UNQUOTED(DSAOBJDIR, "$withval/", [DSA Obj dir])
AC_SUBST(DSAOBJDIR, "$withval")

dnl Set the DSA src directory.
AC_ARG_WITH(dsasrcdir,
  AS_HELP_STRING([--with-dsasrcdir],
    [Specify location of DSA source directory]),
  AC_DEFINE(ENABLE_DSA, [1], [Defined if DSA is available]), )
case "$withval" in
  /* | [[A-Za-z]]:[[\\/]]*)  ;;
  *) AC_MSG_ERROR([Invalid path for --with-dsasrcdir. Provide full path]) ;;
esac
AC_DEFINE_UNQUOTED(DSASRCDIR, "$withval/", [DSA Src dir])
AC_SUBST(DSASRCDIR, "$withval")

dnl --enable-reporting
AC_ARG_ENABLE(reporting, AS_HELP_STRING(
 --enable-reporting,[Enable reporting in passes and benchmarks (default is NO)]),,enableval="no")
if test ${enableval} = "yes" ; then
  AC_SUBST(ENABLE_REPORTING,[[ENABLE_REPORTING=1]])
  AC_DEFINE(ENABLE_REPORTING, [1], [Defined if Reporting is enabled])
fi

dnl Config headers
AC_CONFIG_HEADERS([include/tanger/config.h])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.common)

dnl Configure project makefiles
dnl List every Makefile that exists within your source tree
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(lib/Makefile)
AC_CONFIG_MAKEFILE(lib/tanger/Makefile)
AC_CONFIG_MAKEFILE(tools/Makefile)
AC_CONFIG_MAKEFILE(tools/tmlink/Makefile)

dnl **************************************************************************
dnl * Determine which system we are building on
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for programs.
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for libraries.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for header files.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for library functions.
dnl **************************************************************************

dnl **************************************************************************
dnl * Enable various compile-time options
dnl **************************************************************************

dnl **************************************************************************
dnl * Set the location of various third-party software packages
dnl **************************************************************************

dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************

dnl This must be last
AC_OUTPUT
