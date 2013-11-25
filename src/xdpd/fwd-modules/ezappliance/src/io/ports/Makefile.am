MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = \
	mmap \
	mockup\
	ez_packet_channel \
	vlink	

noinst_LTLIBRARIES = libfwd_module_gnu_linux_io_ports.la

libfwd_module_gnu_linux_io_ports_ladir = $(includedir)/fwd-modules/ezappliance/io/ports

libfwd_module_gnu_linux_io_ports_la_HEADERS = ioport.h 

libfwd_module_gnu_linux_io_ports_la_SOURCES = \
	ioport.cc \
	ioport.h 

libfwd_module_gnu_linux_io_ports_la_LIBADD = \
	mmap/libfwd_module_gnu_linux_io_ports_mmap.la \
	mockup/libfwd_module_gnu_linux_io_ports_mockup.la\
	ez_packet_channel/libfwd_module_gnu_linux_io_ports_ez_packet_channel.la\
	vlink/libfwd_module_gnu_linux_io_ports_vlink.la
