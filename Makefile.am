# $Id: Makefile.am,v 1.7 2015/05/24 19:51:10 reho Exp $

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
	amavisd-milter.spec \
	autoconf.sh \
	config.h.in \
	configure \
	configure.ac

../website/amavisd-milter.html: amavisd-milter/amavisd-milter.8
	nroff -c -mdoc amavisd-milter/amavisd-milter.8 | \
	perl -pe 's/\xC2\xB7/o/g' | sed -e '1s/.*//' -e '$$s/.*//' | \
	man2html -topm 0 -botm 0 -title "amavisd-milter(8) manual page"| \
	sed -e 's;\(http:[^    ]*\);<a href="\1">\1</a>;' \
	    -e 's;<a href="<a href=".*</a>";<a href=";' \
	    -e 's;</a></a>;</a>;' \
	> ../website/amavisd-milter.html

website: ../website/amavisd-milter.html

README: amavisd-milter/amavisd-milter.8
	nroff -mdoc amavisd-milter/amavisd-milter.8 | col -b | \
	sed -e '1s/.*/This is the README for the amavisd-milter(8)./' \
	    -e '$$s/.*//' > README

dist-hook: README
	@find $(distdir) -type d -name CVS -print | xargs rm -rf
	@find $(distdir) -type f -name .cvsignore -print | xargs rm -f

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
