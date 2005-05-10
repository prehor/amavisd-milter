# $Id$

# Distribution files
EXTRA_DIST= \
	Makefile.am \
	Makefile.in

# Header files
noinst_HEADERS=	\
	amavisd-milter.h

# Binaries
sbin_PROGRAMS=	\
	amavisd-milter

# amavisd-milter compiling parameters
amavisd_milter_SOURCES= \
	amavisd.c \
	main.c \
	mlfi.c
amavisd_milter_LDADD= \
	../compat/libcompat.a
amavisd_milter_CPPFLAGS= \
	-I../compat

maintainer-clean-local:
	-rm -f Makefile.in
