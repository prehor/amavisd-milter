# Distribution files
EXTRA_DIST= \
	amavisd-milter.8 \
	Makefile.am \
	Makefile.in

# Header files
noinst_HEADERS=	\
	amavisd-milter.h

# Manual pages
man_MANS= \
	amavisd-milter.8

# Binaries
sbin_PROGRAMS=	\
	amavisd-milter

# amavisd-milter compiling parameters
amavisd_milter_SOURCES= \
	amavisd.c \
	log.c \
	main.c \
	mlfi.c
amavisd_milter_LDADD= \
	../compat/libcompat.a
amavisd_milter_CPPFLAGS= \
	-I../compat

amavisd-milter.8: ../AMAVISD-MILTER.md
	cat ../AMAVISD-MILTER.md | \
	sed "s/^# amavisd-milter(8)/% amavisd-milter(8) Version ${VERSION} | System Manager's Manual/" | \
	sed "s/^#//" | \
	pandoc -s -t man > $@

maintainer-clean-local:
	-rm -f Makefile.in
	-rm -f amavisd-milter.8
