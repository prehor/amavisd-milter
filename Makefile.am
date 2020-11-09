# Subdirectories
SUBDIRS= \
	compat \
	amavisd-milter

DIST_SUBDIRS= \
	${SUBDIRS}


# Not a GNU package
AUTOMAKE_OPTIONS= \
	foreign

# Distribution files
EXTRA_DIST= \
	.editorconfig \
	AMAVISD-MILTER.md \
	CHANGES \
	INSTALL \
	LICENSE \
	Makefile.am \
	Makefile.in \
	aclocal.m4 \
	autoconf.sh \
	config.h.in \
	configure \
	configure.ac

dist-hook:
	cd amavisd-milter; \
	$(MAKE) amavisd-milter.8

maintainer-clean-local:
	-rm -f Makefile.in
	-rm -f aclocal/ar-lib
	-rm -f aclocal/compile
	-rm -f aclocal/config.guess
	-rm -f aclocal/config.sub
	-rm -f aclocal/depcomp
	-rm -f aclocal/install-sh
	-rm -f aclocal/missing
	-rm -f aclocal/mkinstalldirs
	-rm -f aclocal.m4
	-rm -f amavisd-milter-*.tar.gz
	-rm -f config.h.in
	-rm -f config.h.in~
	-rm -f configure
