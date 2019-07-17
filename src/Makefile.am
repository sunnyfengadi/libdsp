lib_LTLIBRARIES = libdsp.la

AM_LDFLAGS = -lmcapi -llo -lasound
AM_CPPFLAGS = -I$(top_srcdir)/include

libdsp_la_SOURCES  = dsp_communication.c
libdsp_la_LDFLAGS = -shared -fpic
libdsp_la_LIBADD    = @DSP_LIBS@
