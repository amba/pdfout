srcdir := $(dir $(lastword $(MAKEFILE_LIST)))

vpath %.h $(srcdir)
vpath %.c $(srcdir)

pdfout_header := $(wildcard $(srcdir)/src/*.h $(srcdir)/src/program/*.h)
pdfout_source := $(wildcard $(srcdir)/src/*.c $(srcdir)/src/program/*.c)
pdfout_obj := $(subst $(srcdir)/,, $(patsubst %.c, %.o, $(pdfout_source)))

pdfout := src/program/pdfout

mupdf-out := mupdf-build
ALL_LDLIBS := $(mupdf-out)/libmupdf.a $(mupdf-out)/libmupdfthird.a -lm

ifneq "$(verbose)" "yes"
quiet_cc = @ echo '   ' CC $@ ;
quiet_link = @ echo '   ' LD $@ ;
endif

all_cc = $(quiet_cc) $(CC)
all_link = $(quiet_link) $(CC)

all_cflags := -std=gnu99 $(CFLAGS)
all_cppflags := -I$(srcdir)/src -I$(srcdir)/mupdf/include $(CPPFLAGS)
COMPILE.c = $(all_cc) $(all_cflags) $(all_cppflags) $(TARGET_ARCH) -c

$(pdfout): $(pdfout_obj)
	$(all_link) $(LDFLAGS) -o $@ $(pdfout_obj) $(ALL_LDLIBS)

objdirs := src src/program

$(objdirs):
	mkdir -p $@

$(pdfout_obj): $(pdfout_header) | $(objdirs) mupdf

# Do not pass down command line arguments to the mupdf make.
# mupdf needs it's own command line arguments like XCFLAGS.
MAKEOVERRIDES :=

mupdf:
	$(MAKE) -C $(srcdir)/mupdf libs third \
	build=debug \
	verbose=$(verbose) \
	OUT=$(CURDIR)/$(mupdf-out) \
	XCFLAGS="$(CFLAGS)" \
	SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS= \


build_doc := perl -I $(srcdir)/doc $(srcdir)/doc/build-doc.pl

html:
	$(build_doc) --build $(srcdir)/doc

clean-obj := $(wildcard src/*.o src/program/*.o) src/program/pdfout \
             $(mupdf-out)

clean:
	rm -rf $(clean-obj)

INSTALL := install
prefix ?= /usr/local
bindir ?= $(prefix)/bin

install: $(pdfout)
	install -d $(DESTDIR)$(bindir)
	install $(pdfout) $(DESTDIR)$(bindir)



.PHONY: all pdfout mupdf check html clean
