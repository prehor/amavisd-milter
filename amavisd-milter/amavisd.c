/*
 * Copyright (c) 2005, Petr Rehor <rx@rx.cz>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: amavisd.c,v 1.2 2005/12/04 23:59:52 reho Exp $
 */

#include "amavisd-milter.h"

#include <ctype.h>
#include <errno.h>



/*
** AMAVISD_INIT - Check amavisd sockect
*/
int
amavisd_init(void)
{
    /* TODO: implement amavisd check */
    /* XXXX: error log processing must be done there */
    return 0;
}


/*
** AMAVISD_CONNECT - Connect to amavisd socket
*/
int
amavisd_connect(struct sockaddr_un *sock)
{
    int		sd;

    /* Initialize domain socket */
    memset(sock, '\0', sizeof(*sock));
    sock->sun_family = AF_UNIX;
    (void) strlcpy(sock->sun_path, amavisd_socket, sizeof(sock->sun_path));
    if ((sd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
	return -1;
    }

    /* Connect to amavisd */
    if (connect(sd, (struct sockaddr *)sock, sizeof(*sock)) == -1) {
	return -1;
    }

    /* Return socket */
    return sd;
}


/*
** AMAVISD_REQUEST - Write request line to amavisd
*/
int
amavisd_request(int sd, char *name, char *value)
{
    char       *b, *p;
    char	buf[MAXAMABUF];

    /* Encode request */
    b = buf;
    if (name != NULL) {
	p = name;
	while (*p != '\0') {
	    if (b >= buf + sizeof(buf) - 8) {
		errno = EOVERFLOW;
		return -1;
	    }
	    if (isalnum(*p) || *p == '-' || *p == '_') {
		*b++ = *p++;
	    } else {
		(void) snprintf(b, 4, "%%%02x", *p++);
		b += 3;
	    }
	}
	*b++ = '=';
    }
    if (name != NULL && value != NULL) {
	p = value;
	while (*p != '\0' && b < buf + sizeof(buf)) {
	    if (b >= buf + sizeof(buf) - 4) {
		errno = EOVERFLOW;
		return -1;
	    }
	    if (isalnum(*p) || *p == '-' || *p == '_') {
		*b++ = *p++;
	    } else {
		(void) snprintf(b, 4, "%%%02x", *p++);
		b += 3;
	    }
	}
    }
    *b++ = '\n';

    /* Write request to amavisd socket */
    return write_sock(sd, buf, b - buf, amavisd_timeout);
}


/*
** AMAVISD_RESPONSE - Read response line from amavisd
*/
int
amavisd_response(int sd, char *line, size_t maxlen)
{
    int		decode = 0;
    char       *p = line;

    /* Read response line */
    while (read_sock(sd, p, 1, amavisd_timeout) > 0) {
	if (p >= line + maxlen - 2) {
	    *(p + 1) = '\0';
	    errno = EOVERFLOW;
	    return -1;
	}
	if (*p == '\n') {
	    *p = '\0';
	    return 0;
	} else if (*p == '%') {
	    decode = 1;
	} else if (decode == 1) {
	    if (isxdigit(*p)) {
		decode = 2;
		p++;
	    } else {
		*(p + 1) = '\0';
		errno = EILSEQ;
		return -1;
	    }
	} else if (decode == 2) {
	    if (isxdigit(*p)) {
		*(p + 1) = '\0';
		*(p - 1) = (u_char) strtol(p - 1, NULL, 16);
		decode = 0;
	    } else {
		*(p + 1) = '\0';
		errno = EILSEQ;
		return -1;
	    }
	} else if (*p == '\r') {
	    /* Do nothing */
	} else {
	    p++;
	}
    }

    /* read_sock failed */
    *p = '\0';
    return -1;
}


/*
** AMAVISD_CLOSE - Close amavisd socket
*/
int
amavisd_close(int sd)
{
    return close(sd);
}
