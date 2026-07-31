SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = slimts.pc

# Include both the pkgconfig template and the version script in tarball distributions
EXTRA_DIST = \
	slimts.pc.in \
	scripts/bump-version.sh

# Maintainer target to calculate and create dynamic Git tags locally
.PHONY: version
version:
	@$(top_srcdir)/scripts/bump-version.sh
