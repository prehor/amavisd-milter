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
 * $Id: amavisd.c,v 1.8 2006/10/07 17:36:19 reho Exp $
 */

#include "amavisd-milter.h"

#include <ctype.h>


/*
** AMAVISD_GROW_AMABUF - Reallocate amavisd communication buffer
*/
static void *
amavisd_grow_amabuf(struct mlfiCtx *mlfi)
{
    char       *amabuf;
    size_t	buflen;

    /* Calculate new buffer size */
    buflen = mlfi->mlfi_amabuf_length + AMABUFCHUNK;
    if (mlfi->mlfi_amabuf_length < MAXAMABUF && buflen >= MAXAMABUF) {
	buflen = MAXAMABUF;
	logqidmsg(mlfi, LOG_NOTICE,
	    "maximum size of amavisd communication buffer was reached");
    } else if (buflen > MAXAMABUF) {
	logqidmsg(mlfi, LOG_ERR,
	    "amavisd communication buffer size is too big (%lu)",
	    (unsigned long)buflen);
	errno = EOVERFLOW;
	return NULL;
    }

    /* Reallocate buffer */
    if ((amabuf = realloc(mlfi->mlfi_amabuf, buflen)) == NULL) {
	logqidmsg(mlfi, LOG_ALERT,
	    "could not reallocate  amavisd communication buffer (%lu)",
	    (unsigned long)buflen);
	return NULL;
    }
    mlfi->mlfi_amabuf = amabuf;
    mlfi->mlfi_amabuf_length = buflen;

    logqidmsg(mlfi, LOG_DEBUG,
	"amavisd communication buffer was increased to %lu",
	(unsigned long)buflen);

    return amabuf;
}


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
amavisd_connect(struct mlfiCtx *mlfi, struct sockaddr_un *sock)
{
    /* Lock amavisd connection */
    if (max_sem != NULL && mlfi->mlfi_max_sem_locked == 0) {
	if (sem_trywait(max_sem) == -1) {
	    if (errno != EAGAIN) {
		logqidmsg(mlfi, LOG_ERR,
		    "could not lock amavisd connections semaphore: %s",
		    strerror(errno));
	    }
	    return -1;
	}
	mlfi->mlfi_max_sem_locked = 1;
    }

    /* Initialize domain socket */
    memset(sock, '\0', sizeof(*sock));
    sock->sun_family = AF_UNIX;
    (void) strlcpy(sock->sun_path, amavisd_socket, sizeof(sock->sun_path));
    if ((mlfi->mlfi_amasd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
	return -1;
    }

    /* Connect to amavisd */
    if (connect(mlfi->mlfi_amasd, (struct sockaddr *)sock, sizeof(*sock)) == -1)
    {
	return -1;
    }

    /* Return socket */
    return mlfi->mlfi_amasd;
}


/*
** AMAVISD_REQUEST - Write request line to amavisd
*/
int
amavisd_request(struct mlfiCtx *mlfi, char *name, char *value)
{
    char       *p;
    char       *b = mlfi->mlfi_amabuf;

    /* Encode request */
    if (name != NULL) {
	p = name;
	while (*p != '\0') {
	    if (b >= mlfi->mlfi_amabuf + mlfi->mlfi_amabuf_length - 5 &&
		amavisd_grow_amabuf(mlfi) == NULL)
	    {
		return -1;
	    }
	    if (isalnum(*p) || *p == '-' || *p == '_') {
		*b++ = *p++;
	    } else {
		(void) snprintf(b, 4, "%%%02x", *p++);
		b += 3;
	    }
	}
	if (value != NULL) {
	    *b++ = '=';
	}
    }
    if (value != NULL) {
	p = value;
	while (*p != '\0') {
	    if (b >= mlfi->mlfi_amabuf + mlfi->mlfi_amabuf_length - 4 &&
		amavisd_grow_amabuf(mlfi) == NULL)
	    {
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
    return write_sock(mlfi->mlfi_amasd, mlfi->mlfi_amabuf,
	b - mlfi->mlfi_amabuf, amavisd_timeout);
}


/*
** AMAVISD_RESPONSE - Read response line from amavisd
*/
int
amavisd_response(struct mlfiCtx *mlfi)
{
    int		decode = 0;
    char       *b = mlfi->mlfi_amabuf;

    /* Read response line */
    while (read_sock(mlfi->mlfi_amasd, b, 1, amavisd_timeout) > 0) {
	if (b >= mlfi->mlfi_amabuf + mlfi->mlfi_amabuf_length - 2 &&
	    amavisd_grow_amabuf(mlfi) == NULL)
	{
	    *(b + 1) = '\0';
	    return -1;
	}
	if (*b == '\n') {
	    *b = '\0';
	    return 0;
	} else if (*b == '%') {
	    decode = 1;
	} else if (decode == 1) {
	    if (isxdigit(*b)) {
		decode = 2;
		b++;
	    } else {
		*(b + 1) = '\0';
		errno = EILSEQ;
		return -1;
	    }
	} else if (decode == 2) {
	    if (isxdigit(*b)) {
		*(b + 1) = '\0';
		*(b - 1) = (u_char) strtol(b - 1, NULL, 16);
		decode = 0;
	    } else {
		*(b + 1) = '\0';
		errno = EILSEQ;
		return -1;
	    }
	} else if (*b == '\r') {
	    /* Do nothing */
	} else {
	    b++;
	}
    }

    /* read_sock failed */
    *b = '\0';
    return -1;
}


/*
** AMAVISD_CLOSE - Close amavisd socket
*/
int
amavisd_close(struct mlfiCtx *mlfi)
{
    int ret = 0;

    /* Unlock amavisd connection */
    if (mlfi->mlfi_max_sem_locked != 0) {
	if ((ret = sem_post(max_sem)) == -1) {
	    logqidmsg(mlfi, LOG_ERR,
		"%s: could not unlock amavisd connections semaphore: %s",
		strerror(errno));
	} else {
	    logqidmsg(mlfi, LOG_DEBUG, "got back amavisd connection");
	}
	mlfi->mlfi_max_sem_locked = 0; 
    }

    /* Close amavisd connection */
    if (mlfi->mlfi_amasd != -1) {
	if (close(mlfi->mlfi_amasd) == -1) {
	    ret = -1;
	}
	mlfi->mlfi_amasd = -1;
    }
    return ret;
}
