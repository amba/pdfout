# Run gnulib tests last (see sec. 3.9 in gnulib manual).
SUBDIRS = . gl src
BUILD_DOC = perl -I "$(srcdir)/doc" "$(srcdir)/doc/build-doc.pl"

all-local:
	+ cd $(srcdir)/mupdf && $(GNU_MAKE) libs third build=debug \
          OUT="$(abs_top_builddir)/mupdf" \
	  $(MUPDF_MAKE_COMMAND) XCFLAGS="$(MUPDF_XCFLAGS)"

clean-local:
	rm -rf "$(abs_top_builddir)/mupdf"
	$(BUILD_DOC) --clean

distclean-local:
	+ cd $(srcdir)/mupdf && $(GNU_MAKE) nuke build=debug \
                                               OUT="$(abs_top_builddir)/mupdf"

dist-hook:
	cp -R $(srcdir)/mupdf $(distdir)/mupdf

# mupdf generates some files inside the source tree.
distcheck-hook:
	chmod -R u+w $(distdir)/mupdf

html-local:
	 $(BUILD_DOC) --build "$(srcdir)/doc"

check-local:
	prove "-I$(srcdir)/test" "$(srcdir)/test/"

EXTRA_DIST = COPYING COPYRIGHT README AUTHORS INSTALL TODO

AM_DISTCHECK_CONFIGURE_FLAGS = --cache-file=$(abs_top_builddir)/config.cache
