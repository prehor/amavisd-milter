# $Id: Makefile.am,v 1.2 2005/12/25 22:15:40 reho Exp $

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
	main.c \
	mlfi.c
amavisd_milter_LDADD= \
	../compat/libcompat.a
amavisd_milter_CPPFLAGS= \
	-I../compat

maintainer-clean-local:
	-rm -f Makefile.in
