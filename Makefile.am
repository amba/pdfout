SUBDIRS = . gl src 

all-local:
	+ cd $(srcdir)/mupdf && $(GNU_MAKE) libs third build=$(mupdf_build)

clean-local:
	+ cd $(srcdir)/mupdf && $(GNU_MAKE) clean build=$(mupdf_build)

distclean-local:
	+ cd $(srcdir)/mupdf && $(GNU_MAKE) nuke build=$(mupdf_build)

dist-hook:
	cp -R $(srcdir)/mupdf $(distdir)/mupdf \
	&& rm -rf $(distdir)/mupdf/build

distcheck-hook:
# mupdf needs to build in source dir, so make it writable
	chmod u+w $(distdir) && chmod -R u+w $(distdir)/mupdf

EXTRA_DIST = COPYING README AUTHORS INSTALL TODO

AM_DISTCHECK_CONFIGURE_FLAGS = --cache-file=$(abs_top_builddir)/config.cache
