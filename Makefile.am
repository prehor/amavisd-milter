# $Id$

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
	AUTHORS \
	CHANGES \
	INSTALL \
	LICENSE \
	README \
	TODO \
	Makefile.am \
	Makefile.in \
	aclocal.m4 \
	config.h.in \
	configure \
	configure.ac

dist-hook:
	@find $(distdir) -type d -name CVS -print | xargs rm -rf
	@find $(distdir) -type f -name .cvsignore -print | xargs rm -f

maintainer-clean-local:
	-rm -f Makefile.in
	-rm -f aclocal/compile
	-rm -f aclocal/depcomp
	-rm -f aclocal/install-sh
	-rm -f aclocal/missing
	-rm -f aclocal/mkinstalldirs
	-rm -f aclocal.m4
	-rm -f amavisd-milter-*.tar.gz
	-rm -f config.h.in
	-rm -f config.h.in~
	-rm -f configure
