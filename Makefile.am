DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

ALWAYS_SUBDIRS =	\
	gst-libs	\
	ext 		\
	tests		\
	common		\
	docs		\
	m4

SUBDIRS = $(ALWAYS_SUBDIRS)

DIST_SUBDIRS = $(ALWAYS_SUBDIRS)

EXTRA_DIST = \
	depcomp \
	AUTHORS NEWS README REQUIREMENTS \
	ChangeLog gst-inference.doap autogen.sh

DISTCLEANFILES = _stdint.h

noinst_HEADERS =

ACLOCAL_AMFLAGS = -I m4 -I common/m4

include $(top_srcdir)/common/release.mak

check-valgrind:
	$(MAKE) -C tests/check check-valgrind

if HAVE_GST_CHECK
check-torture:
	$(MAKE) -C tests/check torture

build-checks:
	$(MAKE) -C tests/check build-checks
else
check-torture:
	true
build-checks:
	true
endif

include $(top_srcdir)/common/coverage/lcov.mak

# cruft: plugins that have been merged or moved or renamed

CRUFT_FILES = \
	$(top_builddir)/common/shave \
	$(top_builddir)/common/shave-libtool

CRUFT_DIRS =

include $(top_srcdir)/common/cruft.mak

all-local: check-cruft

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gst-inference-@PACKAGE_VERSION@.pc

EXTRA_DIST = gst-inference.pc.in