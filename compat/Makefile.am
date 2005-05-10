# $Id$

# Distribution files
EXTRA_DIST= \
	Makefile.am \
	Makefile.in

# Header files
noinst_HEADERS=	\
	compat.h

# Libraries
noinst_LIBRARIES= \
	libcompat.a

# libcompat compiling parameters
libcompat_a_SOURCES= \
	read_sock.c \
	write_sock.c \
	strlcpy.c

maintainer-clean-local:
	-rm -f Makefile.in
