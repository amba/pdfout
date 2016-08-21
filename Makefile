srcdir := $(dir $(lastword $(MAKEFILE_LIST)))

vpath %.h $(srcdir)
vpath %.c $(srcdir)

pdfout_header := $(wildcard $(srcdir)/src/*.h $(srcdir)/src/program/*.h)
pdfout_source := $(wildcard $(srcdir)/src/*.c $(srcdir)/src/program/*.c)
pdfout_obj := $(subst $(srcdir)/,, $(patsubst %.c, %.o, $(pdfout_source)))

pdfout := src/program/pdfout

all: $(pdfout)

$(pdfout): $(pdfout_obj) | mupdf

mupdf-out := mupdf-build
ALL_LDLIBS := $(mupdf-out)/libmupdf.a $(mupdf-out)/libmupdfthird.a -lm

$(pdfout): $(pdfout_obj)
	$(CC) $(LDFLAGS) -o $@ $(pdfout_obj) $(ALL_LDLIBS)

objdirs := src src/program

$(objdirs):
	mkdir -p $@

$(pdfout_obj): $(pdfout_header) | $(objdirs)

# Do not pass down command line arguments to the mupdf make.
# mupdf needs it's own command line arguments like XCFLAGS.
MAKEOVERRIDES :=

mupdf:
	$(MAKE) -C $(srcdir)/mupdf libs third \
	build=debug \
	OUT=$(CURDIR)/$(mupdf-out) \
	XCFLAGS="$(CFLAGS)" \
	SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS= \
	verbose=yes

ALL_CFLAGS := -std=gnu99 $(CFLAGS)
ALL_CPPFLAGS := -I$(srcdir)/src -I$(srcdir)/mupdf/include $(CPPFLAGS)
COMPILE.c := $(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) -c

check:
	echo FIXME; exit 1

build_doc := perl -I $(srcdir)/doc $(srcdir)/doc/build-doc.pl

html:
	$(build_doc) --build $(srcdir)/doc

.PHONY: all pdfout mupdf check html
