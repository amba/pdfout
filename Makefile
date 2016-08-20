SRC_DIR :=
vpath %.c $(SRC_DIR)
vpath %.h $(SRC_DIR)
pdfout_header := $(wildcard $(SRC_DIR)/src/*.h $(SRC_DIR)/src/program/*.h)
pdfout_src := $(wildcard $(SRC_DIR)/src/*.c $(SRC_DIR)/src/program/*.c)
pdfout_obj := $(subst $(SRC_DIR)/,, $(patsubst %.c, %.o, $(pdfout_src)))

all: pdfout

pdfout: mupdf $(pdfout_obj)

ALL_LDLIBS = mupdf/libmupdf.a mupdf/libmupdfthird.a -lm

pdfout: src/program/pdfout

src/program/pdfout: $(pdfout_obj)
	$(CC) $(LDFLAGS) -o $@ $(pdfout_obj) $(ALL_LDLIBS)

objdirs := src src/program

$(objdirs):
	mkdir -p $@

$(pdfout_obj): | $(objdirs)




mupdf:
	$(MAKE) -C $(SRC_DIR)/mupdf libs third build=debug \
OUT=$(CURDIR)/mupdf XCFLAGS="$(CFLAGS)" SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS= \
verbose=yes

ALL_CFLAGS = -std=gnu99 $(CFLAGS)
ALL_CPPFLAGS = -I$(SRC_DIR)/src -I$(SRC_DIR)/mupdf/include $(CPPFLAGS)
COMPILE.c = $(CC) $(ALL_CFLAGS) $(ALL_CPPFLAGS) $(TARGET_ARCH) -c

check: all
	prove -I$(SRC_DIR)/test $(SRC_DIR)/test/

.PHONY: all pdfout mupdf check
