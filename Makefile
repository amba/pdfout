OUT := build

pdfout_header := $(wildcard src/*.h src/program/*.h)
pdfout_source := $(wildcard src/*.c src/program/*.c)
pdfout_obj := $(addprefix $(OUT)/, $(patsubst %.c, %.o, $(pdfout_source)))

# We need to rebuild, if the mupdf headers change.
mupdf_inc := mupdf/include/mupdf
mupdf_header := $(wildcard $(mupdf_inc)/*.h $(mupdf_inc)/pdf/*.h \
                           $(mupdf_inc)/fitz/*.h)

all_header := $(pdfout_header) $(mupdf_header)

pdfout := $(OUT)/pdfout

mupdf_out := $(OUT)/mupdf

mupdf_libs := $(mupdf_out)/libmupdf.a $(mupdf_out)/libmupdfthird.a
ALL_LDLIBS :=  $(mupdf_libs) -lm

ifneq "$(verbose)" "yes"
quiet_cc = @ echo '   ' CC $@ ;
quiet_link = @ echo '   ' LD $@ ;
endif

# Do not touch user variables $(CC), $(CFLAGS), ...
# They must always come last, to give maximum control to the command line.

all_cc = $(quiet_cc) $(CC)
all_link = $(quiet_link) $(CC)
all_cflags := -std=gnu99 $(CFLAGS)
all_cppflags := -Isrc -Imupdf/include $(CPPFLAGS)

$(pdfout): $(pdfout_obj)
	$(all_link) $(LDFLAGS) -o $@ $(pdfout_obj) $(ALL_LDLIBS)

COMPILE.c = $(all_cc) $(all_cflags) $(all_cppflags) $(TARGET_ARCH) -c

objdirs := $(OUT)/src $(OUT)/src/program

$(OUT)/%.o: %.c $(all_header) | $(objdirs)
	$(COMPILE.c) $< -o $@


$(objdirs):
	mkdir -p $@

clean-obj := $(wildcard $(OUT)/src/*.o $(OUT)/src/program/*.o) $(OUT)/pdfout \
             $(mupdf_out)

clean:
	rm -rf $(clean-obj)

INSTALL := install
prefix ?= /usr/local
bindir ?= $(prefix)/bin

install: $(pdfout)
	install -d $(DESTDIR)$(bindir)
	install $(pdfout) $(DESTDIR)$(bindir)



.PHONY: all clean
