MAINTAINERCLEANFILES = Makefile.in

#Do first the config
SUBDIRS = afa-imp io util pipeline-imp ezappliance idl

noinst_LTLIBRARIES = libfwd_module_gnu_linux_src.la

library_includedir=$(includedir)/fwd-modules/ezappliance
include_HEADERS=

libfwd_module_gnu_linux_src_la_SOURCES = \
	config.cc\
	bg_taskmanager.cc \
	ls_internal_state.h

libfwd_module_gnu_linux_src_la_LIBADD = \
                idl/libfwd_module_gnu_linux_idl.la \
		ezappliance/libfwd_module_gnu_linux_ezappliance.la \
		afa-imp/libfwd_module_gnu_linux_afa_imp.la \
		io/libfwd_module_gnu_linux_io.la \
		pipeline-imp/libfwd_module_gnu_linux_pipeline_imp.la \
		util/libfwd_module_gnu_linux_util.la
		
		
library_include_HEADERS=


