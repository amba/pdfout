AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/gl -I$(top_builddir)/gl \
  -I$(top_builddir)

AM_CPPFLAGS += -I$(top_srcdir)/mupdf/include

AM_CFLAGS = $(WARN_CFLAGS)

LIBS = $(MUPDF_REQUIRED_LIBS)

bin_PROGRAMS = pdfout

pdfout_SOURCES = \
pdfout.c \
commands.def \
shared.c \
shared.h \
debug-io-handle.c \
debug-io-handle.h \
getoutline.c \
setoutline.c \
getinfo.c \
setinfo.c \
pagecount.c \
getpagelabels.c \
setpagelabels.c \
gettxt.c \
repair.c \
create.c \
debug.c \
test.h \
test.c


LDADD = \
../libcommon.a \
../libshared-with-tests.a \
$(top_builddir)/gl/libgnu.a \
$(abs_top_builddir)/mupdf/libmupdf.a \
$(abs_top_builddir)/mupdf/libmupdfthird.a \
$(abs_top_builddir)/mupdf/libmupdf.a
