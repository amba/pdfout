SUBDIRS =  . program

AM_CPPFLAGS = -I$(top_srcdir)/gl -I$(top_builddir)/gl -I$(top_builddir)
AM_CPPFLAGS += -I$(top_srcdir)/mupdf/include

AM_CFLAGS = $(WARN_CFLAGS)

noinst_LIBRARIES = libcommon.a libshared-with-tests.a
libcommon_a_SOURCES = \
resolve-dest.c \
misc.c \
common.h \
outline.c \
page-labels.h \
page-labels.c \
info-dict.c \
info-dict.h \
charset-conversion.c \
charset-conversion.h \
open-write-pdf.c \
text-extraction.c \
alloc.c \
data.c \
data.h \
json.c \
tmp-stream.h \
tmp-stream.c

libshared_with_tests_a_SOURCES = \
shared-with-tests.h \
shared-with-tests.c

EXTRA_DIST = \
pdfdoc.txt \
PDFDOCENCODING.TXT \
encodings.gperf
