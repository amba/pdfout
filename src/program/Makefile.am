AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/gl -I$(top_builddir)/gl \
  -I$(top_builddir)

AM_CPPFLAGS += -I$(top_srcdir)/mupdf/include

AM_CFLAGS = $(WARN_CFLAGS)

LIBS = $(MUPDF_REQUIRED_LIBS)
noinst_PROGRAMS = bug-xref-incremental
bin_PROGRAMS = pdfout
pdfout_SOURCES = \
  pdfout.c \
  commands.def \
  shared.c \
  shared.h \
  getoutline.c \
  setoutline.c \
  convert-outline.c \
  getinfo.c \
  setinfo.c \
  pagecount.c \
  getpagelabels.c \
  setpagelabels.c \
  gettxt.c \
  repair.c \
  create.c \
  debug.c

LDADD = \
../libcommon.a \
../libshared-with-tests.a \
$(top_builddir)/gl/libgnu.a

mupdf_lib_dir = $(top_srcdir)/mupdf/build/$(mupdf_build)
LDADD += \
  $(mupdf_lib_dir)/libmupdf.a \
  $(mupdf_lib_dir)/libfreetype.a \
  $(mupdf_lib_dir)/libjbig2dec.a \
  $(mupdf_lib_dir)/libopenjpeg.a \
  $(mupdf_lib_dir)/libjpeg.a \
  $(mupdf_lib_dir)/libz.a

