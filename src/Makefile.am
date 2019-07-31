lib_LTLIBRARIES = libdsp.la
libdsp_la_SOURCES  = dsp_communication.c adi_xml_parse.c

AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/include
AM_CPPFLAGS += $(XML_CFLAGS)

AM_LDFLAGS = -module -avoid-version -export-dynamic -no-undefined $(LDFLAGS_NOUNDEFINED)

libdsp_la_LIBADD   = -lpthread -lasound -llo -lmcapi
libdsp_la_LIBADD   += $(XML_LIBS)
