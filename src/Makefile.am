SUBDIRS =  . program test 

AM_CPPFLAGS = -I$(top_srcdir)/gl -I$(top_builddir)/gl -I$(top_builddir)
AM_CPPFLAGS += -I$(top_srcdir)/mupdf/include

AM_CFLAGS = $(WARN_CFLAGS)

noinst_LIBRARIES = libcommon.a libshared-with-tests.a
libcommon_a_SOURCES = \
misc.c \
common.h \
outline-wysiwyg.c \
outline-wysiwyg.h \
libyaml-wrappers.c \
outline.c \
page-labels.h \
page-labels.c \
pdf-page-labels.c \
info-dict.c dates.c \
string-conversion.c \
open-write-pdf.c \
text-extraction.c \
pdfout-regex.h \
regex.c

libshared_with_tests_a_SOURCES = \
shared-with-tests.h \
shared-with-tests.c

EXTRA_DIST = \
pdfdoc.txt \
PDFDOCENCODING.TXT

