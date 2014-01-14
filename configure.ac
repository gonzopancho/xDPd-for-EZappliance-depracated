AC_INIT(xdpd, 0.3, xdpd@bisdn.de, xdpd)
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_GNU_SOURCE

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LD
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AUTOCONF_ENV="SHELL=/bin/sh"
LT_INIT
link_all_deplibs=yes
link_all_deplibs_CXX=yes
AC_ENABLE_STATIC
LIBTOOL="$LIBTOOL --preserve-dup-deps"
AC_SUBST(LIBTOOL)dnl


<<<<<<< HEAD
# Application verbosity 
AC_MSG_CHECKING(whether to print debug in verbose mode)
AC_ARG_ENABLE(verbose,
	AS_HELP_STRING([--verbose], [turn on verbose mode [default=no]])
		, verbose_debug="yes", verbose_debug="no")
if test "$verbose_debug" = "yes"; then
	CFLAGS="$CFLAGS -DVERBOSE_DEBUG" 
	CXXFLAGS="$CXXFLAGS -DVERBOSE_DEBUG" 
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

# Check for profiling mode
AC_MSG_CHECKING(whether to enable profiling mode)
profile_default="no"
AC_ARG_ENABLE(profile,
	AS_HELP_STRING([--enable-profile], [turn on profile mode [default=no]])
		, , enable_profile=$profile_default)
if test "$enable_profile" = "yes"; then
	CFLAGS="$( echo $CFLAGS | sed s/-fomit-frame-pointer//g )"
	CXXFLAGS="$( echo $CXXFLAGS | sed s/-fomit-frame-pointer//g )"
	CFLAGS="$CFLAGS -pg"
	CXXFLAGS="$CXXFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

#############################
# CHECKING Hardware support #
#############################
AC_MSG_CHECKING(hardware support to be compiled)

AC_ARG_ENABLE(gnu-linux,
	AS_HELP_STRING([--enable-gnu-linux], [compile GNU/Linux hardware support [default=no]])
			, , enable_gnu_linux="no")
AC_ARG_ENABLE(gnu-linux-dpdk,
	AS_HELP_STRING([--enable-gnu-linux-dpdk], [compile GNU/Linux Intel DPDK support [default=no]])
			, , enable_gnu_linux_dpdk="no")
AC_ARG_ENABLE(bcm,
	AS_HELP_STRING([--enable-bcm], [compile Broadcom hardware support [default=no]])
			, , enable_bcm="no")
AC_ARG_ENABLE(octeon,
	AS_HELP_STRING([--enable-octeon], [compile OCTEON 5650 hardware support [default=no]])
			, , enable_octeon="no")
AC_ARG_ENABLE(netfpga10g,
	AS_HELP_STRING([--enable-netfpga10g], [compile NetFPGA 10G hardware code (HW code not included) [default=no]])
			, , enable_netfpga10g="no")
AC_ARG_ENABLE(example,
	AS_HELP_STRING([--enable-example], [compile the Forwarding Module example code [default=no]])
			, , enable_example="no")
#[+] Add your platform here


#Make default GNU/Linux if not defined. 
#[+] Makesure you add your platform here
if ( test "$enable_gnu_linux_dpdk" = "no" ) &&
   ( test "$enable_gnu_linux" = "no" ) &&
   ( test "$enable_bcm" = "no" ) &&
   ( test "$enable_octeon" = "no" ) &&
   ( test "$netfpga10g" = "no" ) &&
   ( test "$enable_example" = "no" ); then
   
   enable_gnu_linux="yes"
=======
#Default flags
CFLAGS="-Werror -Wall -Wl,--as-needed -Wl,-export-dynamic $CFLAGS"
CXXFLAGS="-Werror -Wall -Wl,--as-needed -Wl,-export-dynamic $CXXFLAGS"
AC_DEFINE(__STDC_FORMAT_MACROS)
>>>>>>> 81344d1e893ef62e6c866e5b5ef2bcfc1b7285a4

#Check libs used
AC_CHECK_LIB(pthread, pthread_kill,,AC_MSG_ERROR([pthread library not found])) 
AC_CHECK_LIB(rt, clock_gettime,,[AC_MSG_ERROR([rt library not found])]) 
#AC_CHECK_LIB(rofl_pipeline, physical_switch_init,,[AC_MSG_ERROR([rofl_pipeline library not found])]) 
#AC_CHECK_LIB(rofl_pipeline, physical_switch_init,,[AC_MSG_ERROR([rofl_core library not found])]) 

PKG_CHECK_MODULES([ROFL], [rofl >= 0.3.0], [have_rofl=yes], [have_rofl=no])
if (test "${have_rofl}" = "yes"); then
        CPPFLAGS+="$ROFL_CFLAGS"
        LIBS+="$ROFL_LIBS"
fi
<<<<<<< HEAD
if( test "$enable_bcm" = "yes" );then
	msg="$msg Broadcom"
	AC_DEFINE(ENABLE_BCM)
	PLATFORM=bcm
	AC_CONFIG_SUBDIRS([src/xdpd/fwd-modules/bcm])
fi	
if( test "$enable_octeon" = "yes" );then
	msg="$msg OCTEON 5650"
	AC_DEFINE(ENABLE_OCTEON5650)
	PLATFORM=octeon5650
	AC_CONFIG_SUBDIRS([src/xdpd/fwd-modules/octeon5650])
fi
if( test "$enable_netfpga10g" = "yes" );then
	msg="$msg NetFPGA-10G"
	AC_DEFINE(ENABLE_NETFPGA10G)
	PLATFORM=netfpga10g
	AC_CONFIG_SUBDIRS([src/xdpd/fwd-modules/netfpga10g])
fi
if( test "$enable_example" = "yes" );then
	msg="$msg Example platform"
	AC_DEFINE(ENABLE_EXAMPLE)
	PLATFORM=example
	AC_CONFIG_SUBDIRS([src/xdpd/fwd-modules/example])
fi
#[+]Add your platform here...

=======
>>>>>>> 81344d1e893ef62e6c866e5b5ef2bcfc1b7285a4


#Debug
m4_include([config/debug.m4])

# Application verbosity 
m4_include([config/verbose.m4])

# Check for profiling mode
m4_include([config/profiling.m4])

#Plugin main m4 macros
m4_include([config/plugins.m4])

#Check hardware support
m4_include([config/hw.m4])

#Check hardware support
m4_include([config/versioning.m4])

#Output the files
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/xdpd/Makefile
	src/xdpd/fwd-modules/Makefile
	src/xdpd/management/Makefile
	src/xdpd/openflow/Makefile
	src/xdpd/openflow/openflow10/Makefile
	src/xdpd/openflow/openflow12/Makefile
	test/Makefile
	test/grouptest/Makefile
])

# Doxygen (here to be the last Makefile) 
m4_include([config/doxygen.m4])

AC_OUTPUT
