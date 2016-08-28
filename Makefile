srcdir := $(dir $(lastword $(MAKEFILE_LIST)))

vpath %.h $(srcdir)
vpath %.c $(srcdir)

pdfout_header := $(wildcard $(srcdir)/src/*.h $(srcdir)/src/program/*.h)
pdfout_source := $(wildcard $(srcdir)/src/*.c $(srcdir)/src/program/*.c)
pdfout_obj := $(subst $(srcdir)/,, $(patsubst %.c, %.o, $(pdfout_source)))

mupdf-dir := $(srcdir)/mupdf

# We need to rebuild, if the mupdf headers change.
mupdf-inc := $(mupdf-dir)/include/mupdf
mupdf_header := $(wildcard $(mupdf-inc)/*.h $(mupdf-inc)/pdf/*.h \
                           $(mupdf-inc)/fitz/*.h)

all_header := $(pdfout_header) $(mupdf_header)

pdfout := src/program/pdfout

mupdf-out := mupdf-build
mupdf-libs := $(mupdf-out)/libmupdf.a $(mupdf-out)/libmupdfthird.a
ALL_LDLIBS :=  $(mupdf-libs) -lm

ifneq "$(verbose)" "yes"
quiet_cc = @ echo '   ' CC $@ ;
quiet_link = @ echo '   ' LD $@ ;
endif

# Do not touch user variables $(CC), $(CFLAGS), ...
# They must always come last, do give maximum control to the command line.

all_cc = $(quiet_cc) $(CC)
all_link = $(quiet_link) $(CC)
all_cflags := -std=gnu99 $(CFLAGS)
all_cppflags := -I$(srcdir)/src -I$(srcdir)/mupdf/include $(CPPFLAGS)

COMPILE.c = $(all_cc) $(all_cflags) $(all_cppflags) $(TARGET_ARCH) -c

$(pdfout): $(pdfout_obj) $(mupdf-libs)
	$(all_link) $(LDFLAGS) -o $@ $(pdfout_obj) $(ALL_LDLIBS)

objdirs := src src/program

$(objdirs):
	mkdir -p $@

$(pdfout_obj): $(all_header) | $(objdirs)

# Do not pass down command line arguments to the mupdf make.
# mupdf needs it's own command line arguments like XCFLAGS.
MAKEOVERRIDES :=

# Always rebuild $(mupdf-libs).
# Use pattern rule, otherwise the recipe would be run twice.
# We cannot make $(mupdf-libs) phony, as phony targets are not searched by
# pattern rules. Only libmupdf.a depends on FORCE. Otherwise, the submake gets
# run twice.
$(mupdf-out)/libmupdf.a: FORCE
libmupdf%.a:
	$(MAKE) -C $(mupdf-dir) libs third \
	build=debug \
	verbose=$(verbose) \
	OUT=$(CURDIR)/$(mupdf-out) \
	XCFLAGS="$(CFLAGS)" \
	SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS=

FORCE: ;

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



.PHONY: all pdfout check html clean FORCE
