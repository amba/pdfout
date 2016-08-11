AM_CPPFLAGS = -I$(top_srcdir)/gl -I$(top_builddir)/gl -I$(top_builddir)
AM_CFLAGS = $(WARN_CFLAGS)

LDADD = \
libtest.a \
../libshared-with-tests.a \
$(top_builddir)/gl/libgnu.a

AM_CPPFLAGS += -DTEST_DATA_DIR=$(srcdir)

prog_name = ../program/pdfout
AM_CPPFLAGS += -DTEST_PROG_NAME=$(prog_name)

noinst_PROGRAMS = c-quote

check_LIBRARIES = libtest.a
libtest_a_SOURCES = \
test.c test.h \
test-execute.c test-execute.h

check-bin: $(check_PROGRAMS)

TESTS = $(check_PROGRAMS)

check_PROGRAMS= \
outline-json \
page-labels \
info-dict \
page-count \
repair \
gettxt-plain \
create \
whitebox-tests \
top-level

EXTRA_DIST = \
set-get-template.c \
valgrind.supp \
data/empty10.pdf \
data/broken.pdf \
data/hello-world.pdf \
data/page-labels-broken.pdf \
data/page-labels-really-broken.pdf \
data/outline-yaml.h \
data/outline-wysiwyg.h \
data/gettxt.h
