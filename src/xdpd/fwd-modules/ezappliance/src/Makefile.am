MAINTAINERCLEANFILES = Makefile.in

#Do first the config
SUBDIRS = afa-imp io util pipeline-imp processing ezappliance

noinst_LTLIBRARIES = libfwd_module_gnu_linux_src.la

library_includedir=$(includedir)/fwd-modules/ezappliance
include_HEADERS=

libfwd_module_gnu_linux_src_la_SOURCES = \
	config.cc\
	bg_taskmanager.cc

libfwd_module_gnu_linux_src_la_LIBADD = \
		afa-imp/libfwd_module_gnu_linux_afa_imp.la \
		io/libfwd_module_gnu_linux_io.la \
		ezappliance/libfwd_module_gnu_linux_ezappliance.la \
		pipeline-imp/libfwd_module_gnu_linux_pipeline_imp.la \
		processing/libfwd_module_gnu_linux_processing.la \
		util/libfwd_module_gnu_linux_util.la \
		afa-imp/libfwd_module_gnu_linux_afa_imp.la
		
library_include_HEADERS=


