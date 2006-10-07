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
 * $Id: mlfi.c,v 1.14 2006/10/04 20:46:55 reho Exp $
 */

#include "amavisd-milter.h"

#include <arpa/inet.h>
#include <netinet/in.h>


/*
** SMFILTER - Milter description
*/
struct smfiDesc smfilter =
{
    PACKAGE,			/* filter name */
    SMFI_VERSION,		/* version code -- do not change */
    SMFIF_ADDHDRS |
    SMFIF_CHGHDRS |
    SMFIF_ADDRCPT |
    SMFIF_DELRCPT,		/* filter actions */
    mlfi_connect,		/* connection info filter */
    mlfi_helo,			/* SMTP HELO command filter */
    mlfi_envfrom,		/* envelope sender filter */
    mlfi_envrcpt,		/* envelope recipient filter */
    mlfi_header,		/* header filter */
    mlfi_eoh,			/* end of header */
    mlfi_body,			/* body block filter */
    mlfi_eom,			/* end of message */
    mlfi_abort,			/* message aborted */
    mlfi_close			/* connection cleanup */
};


/*
** SMFI_SETREPLY - Set SMTP reply
*/
#define SMFI_SETREPLY(rcode, xcode, reason) \
{ \
    if (smfi_setreply(ctx, rcode, xcode, reason) != MI_SUCCESS) { \
	logqiderr(mlfi, __func__, LOG_WARNING, \
	    "could not set SMTP reply: %s %s %s", rcode, xcode, reason); \
    } else { \
	logqidmsg(mlfi, LOG_DEBUG, "set reply %s %s %s", rcode, xcode, reason); \
    } \
}


/*
** SMFI_SETREPLY_TEMPFAIL - Set SMFIS_TEMPFAIL reply
*/
#define SMFI_SETREPLY_TEMPFAIL() \
{ \
    SMFI_SETREPLY("451", "4.6.0", "Content scanner malfunction"); \
}


/*
** AMAVISD_REQUEST - Sent request line to amavisd
*/
#define AMAVISD_REQUEST(name, value) \
{ \
    if (name != NULL) { \
	logqidmsg(mlfi, LOG_DEBUG, "%s=%s", name, value); \
    } \
    if (amavisd_request(mlfi, sd, name, value) == -1) {  \
	logqiderr(mlfi, __func__, LOG_CRIT, \
	    "could not write to socket %s: %s", amavisd_socket, \
	    strerror(errno)); \
	SMFI_SETREPLY_TEMPFAIL(); \
	(void) amavisd_close(sd); \
	return SMFIS_TEMPFAIL; \
    } \
}


/*
** AMAVISD_RESPONSE - Read response line from amavisd
*/
#define AMAVISD_RESPONSE(sd) \
    amavisd_response(mlfi, sd)


/*
** AMAVISD_PARSE_RESPONSE - Parse amavisd response line
*/
#define AMAVISD_PARSE_RESPONSE(item, value, sep) \
{ \
    item = value; \
    if ((value = strchr(value, sep)) == NULL) { \
	logqiderr(mlfi, __func__, LOG_ERR, "malformed line: %s", name); \
	SMFI_SETREPLY_TEMPFAIL(); \
	(void) amavisd_close(sd); \
	return SMFIS_TEMPFAIL; \
    } \
    *value++ = '\0'; \
}


/*
** MLFI_CHECK_CTX - Check milter private data
*/
#define MLFI_CHECK_CTX() \
{ \
    if (mlfi == NULL) { \
	logqiderr(mlfi, __func__, LOG_CRIT, "context is not set"); \
	SMFI_SETREPLY_TEMPFAIL(); \
	return SMFIS_TEMPFAIL; \
    } \
}


/*
** MLFI_FREE - Free allocated memory
*/
#define MLFI_FREE(p) \
{ \
    free(p); \
    p = NULL; \
}


/*
** MLFI_STRDUP - Duplicate string
*/
#define MLFI_STRDUP(strnew, str) \
{ \
    if ((str) != NULL && *(str) != '\0') { \
	if ((strnew = strdup(str)) == NULL) { \
	    logqiderr(mlfi, __func__, LOG_ALERT, "could not allocate memory"); \
	    SMFI_SETREPLY_TEMPFAIL(); \
	    return SMFIS_TEMPFAIL; \
	} \
    } \
}


/*
** MLFI_CLEANUP - Cleanup connection context
*/
#define MLFI_CLEANUP(mlfi) \
{ \
	mlfi_cleanup(mlfi); \
	mlfi = NULL; \
}


/*
** MLFI_CLEANUP_MESSAGE - Cleanup message context
**
** mlfi_cleanup_message() close message file if open, unlink work directory
** and release message context
*/
static void
mlfi_cleanup_message(struct mlfiCtx *mlfi)
{
    FTS	       *fts;
    FTSENT     *ftsent;
    char * const wrkdir[] = { mlfi->mlfi_wrkdir, 0 };
    struct	mlfiAddress *rcpt;

    /* Check milter private data */
    if (mlfi == NULL) {
	logqiderr(mlfi, __func__, LOG_DEBUG, "context is not set");
	return;
    }

    logqidmsg(mlfi, LOG_INFO, "CLEANUP");

    /* Unlock amavisd connections semaphore */
    if (mlfi->mlfi_max_sem_locked != 0 && sem_post(max_sem) == -1) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not unlock amavisd "
	    "connections semaphore: %s", strerror(errno));
    }
    mlfi->mlfi_max_sem_locked = 0;

    /* Close the message file */
    if (mlfi->mlfi_fp != NULL) {
	if (fclose(mlfi->mlfi_fp) != 0 && errno != EBADF) {
	    logqiderr(mlfi, __func__, LOG_WARNING, "could not close message "
		"file %s: %s", mlfi->mlfi_fname, strerror(errno));
	} else {
	    logqidmsg(mlfi, LOG_DEBUG, "close message file %s",
		mlfi->mlfi_fname);
	}
	mlfi->mlfi_fp = NULL;
    }

    /* Remove work directory */
    if (mlfi->mlfi_wrkdir[0] != '\0') {
	fts = fts_open(wrkdir, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
	if (fts == NULL) {
	    logqiderr(mlfi, __func__, LOG_ERR, "could not open file hierarchy "
		"%s: %s", mlfi->mlfi_wrkdir, strerror(errno));
	} else {
	    while ((ftsent = fts_read(fts)) != NULL) {
		switch (ftsent->fts_info) {
		case FTS_ERR:
		    /*
		     * This is an error return, and the fts_errno
		     * field will be set to indicate what caused the
		     * error.
		     */
		    logqiderr(mlfi, __func__, LOG_ERR, "could not traverse "
			"file hierarchy %s: %s", ftsent->fts_path,
			strerror(ftsent->fts_errno));
		    break;
		case FTS_DNR:
		    /*
		     * Assume that since fts_read() couldn't read the
		     * directory, it can't be removed.
		     */
		    if (ftsent->fts_errno != ENOENT) {
			logqiderr(mlfi, __func__, LOG_ERR, "could not remove "
			    "directory %s: %s", ftsent->fts_path,
			    strerror(ftsent->fts_errno));
		    }
		    break;
		case FTS_NS:
		    /*
		     * Assume that since fts_read() couldn't stat the
		     * file, it can't be unlinked.
		     */
		    logqiderr(mlfi, __func__, LOG_ERR, "could not unlink file "
			"%s: %s", ftsent->fts_path,
			strerror(ftsent->fts_errno));
		    break;
		case FTS_D:
		    /*
		     * Skip pre-order directory.
		     */
		    break;
		case FTS_DP:
		    /*
		     * Remove post-order directory.
		     */
		    if (rmdir(ftsent->fts_accpath) != 0 && errno != ENOENT) {
			logqiderr(mlfi, __func__, LOG_ERR, "could not remove "
			    "directory %s: %s", ftsent->fts_path,
			    strerror(ftsent->fts_errno));
		    } else {
			logqidmsg(mlfi, LOG_DEBUG, "remove directory %s",
			    ftsent->fts_path);
		    }
		    break;
		default:
		    /*
		     * A regular file or symbolic link.
		     */
		    if (unlink(ftsent->fts_accpath) != 0 && errno != ENOENT) {
			logqiderr(mlfi, __func__, LOG_ERR, "could not unlink "
			    "file %s: %s", ftsent->fts_path,
			    strerror(ftsent->fts_errno));
		    } else {
			logqidmsg(mlfi, LOG_DEBUG, "unlink file %s",
			    ftsent->fts_path);
		    }
		}
	    }
	    if (fts_close(fts) != 0) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not close file "
		    "hirerachy %s: %s", mlfi->mlfi_wrkdir, strerror(errno));
	    }
	}
	mlfi->mlfi_wrkdir[0] = '\0';
    }

    /* Free memory */
    MLFI_FREE(mlfi->mlfi_prev_qid);
    mlfi->mlfi_prev_qid = mlfi->mlfi_qid;
    MLFI_FREE(mlfi->mlfi_from);
    while(mlfi->mlfi_rcpt != NULL) {
	rcpt = mlfi->mlfi_rcpt;
	mlfi->mlfi_rcpt = rcpt->q_next;
	free(rcpt);
    }
}


/*
** MLFI_CLEANUP - Cleanup connection context
**
** mlfi_cleanup() cleanup message context and relese connection context
*/
static void
mlfi_cleanup(struct mlfiCtx *mlfi)
{
    /* Check milter private data */
    if (mlfi == NULL) {
	logqiderr(mlfi, __func__, LOG_DEBUG, "context is not set");
	return;
    }

    /* Cleanup the message context */
    mlfi_cleanup_message(mlfi);

    logqidmsg(mlfi, LOG_INFO, "cleanup connection context");

    /* Cleanup the connection context */
    MLFI_FREE(mlfi->mlfi_addr);
    MLFI_FREE(mlfi->mlfi_hostname);
    MLFI_FREE(mlfi->mlfi_helo);
    MLFI_FREE(mlfi->mlfi_amabuf);
    MLFI_FREE(mlfi->mlfi_prev_qid);

    /* Free context */
    free(mlfi);
}


/*
** MLFI_CONNECT - Handle incomming connection
** 
** mlfi_connect() is called once, at the start of each SMTP connection
*/
sfsistat
mlfi_connect(SMFICTX *ctx, char *hostname, _SOCK_ADDR * hostaddr)
{
    struct	mlfiCtx *mlfi = NULL;
    char       *addr;

    logqidmsg(mlfi, LOG_INFO, "CONNECT: %s", hostname);

    /* Check amavisd socket */
    if (amavisd_init() == -1) {
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }

    /* Allocate memory for private data */
    mlfi = malloc(sizeof(*mlfi));
    if (mlfi == NULL) {
        logqiderr(mlfi, __func__, LOG_ALERT, "could not allocate memory");
        SMFI_SETREPLY_TEMPFAIL();
        return SMFIS_TEMPFAIL;
    }
    (void) memset(mlfi, '\0', sizeof(*mlfi));

    /* Save connection informations */
    MLFI_STRDUP(mlfi->mlfi_hostname, hostname);
    if (hostaddr != NULL) {
	addr = inet_ntoa(((struct sockaddr_in *)hostaddr)->sin_addr);
	MLFI_STRDUP(mlfi->mlfi_addr, addr);
    }

    /* Allocate amavisd communication buffer */
    mlfi->mlfi_amabuf_length = AMABUFCHUNK;
    if ((mlfi->mlfi_amabuf = malloc(mlfi->mlfi_amabuf_length)) == NULL) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not allocate amavisd "
	    "communication buffer");
	SMFI_SETREPLY_TEMPFAIL();
	MLFI_CLEANUP(mlfi);
	return SMFIS_TEMPFAIL;
    }

    /* Save the private data */
    if (smfi_setpriv(ctx, mlfi) != MI_SUCCESS) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not set milter context");
	SMFI_SETREPLY_TEMPFAIL();
	MLFI_CLEANUP(mlfi);
	return SMFIS_TEMPFAIL;
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_HELO - Handle the HELO/EHLO command
**
** mlfi_helo() is called whenever the client sends a HELO/EHLO command.
** It may therefore be called between zero and three times.
*/
sfsistat
mlfi_helo(SMFICTX *ctx, char* helohost)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_DEBUG, "HELO: %s", helohost);

    /* Save helo hostname */
    if (helohost != NULL && *helohost != '\0') {
	MLFI_FREE(mlfi->mlfi_helo);
	MLFI_STRDUP(mlfi->mlfi_helo, helohost);
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_ENVFORM - Handle the envelope FROM command
** 
** mlfi_envfrom() is called once at the beginning of each message, before
** mlfi_envrcpt()
*/
sfsistat
mlfi_envfrom(SMFICTX *ctx, char **envfrom)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);
    char       *qid, *wrkdir;

    /* Check milter private data */
    MLFI_CHECK_CTX();

    /* Cleanup message data */
    mlfi_cleanup_message(mlfi);

    /* Get message id */
    if ((qid = smfi_getsymval(ctx, "i")) != NULL) {
	MLFI_STRDUP(mlfi->mlfi_qid, qid);
    }

    logqidmsg(mlfi, LOG_INFO, "MAIL FROM: %s", *envfrom);

    /* Save from mail address */
    MLFI_STRDUP(mlfi->mlfi_from, *envfrom);

    /* Create work directory */
    if (mlfi->mlfi_qid != NULL) {
	(void) snprintf(mlfi->mlfi_wrkdir, sizeof(mlfi->mlfi_wrkdir) - 1,
	    "%s/af%s", work_dir, mlfi->mlfi_qid);
	if (mkdir(mlfi->mlfi_wrkdir, S_IRWXU|S_IRGRP|S_IXGRP) != 0) {
	    mlfi->mlfi_wrkdir[0] = '\0';
	}
    }
    if (mlfi->mlfi_wrkdir[0] == '\0') {
	(void) snprintf(mlfi->mlfi_wrkdir, sizeof(mlfi->mlfi_wrkdir) - 1,
	    "%s/afXXXXXXXXXX", work_dir);
	if ((wrkdir = mkdtemp(mlfi->mlfi_wrkdir)) != NULL) {
	    (void) strlcpy(mlfi->mlfi_wrkdir, wrkdir,
		sizeof(mlfi->mlfi_wrkdir));
	} else {
	    logqiderr(mlfi, __func__, LOG_ERR, "could not create work "
		"directory: %s", strerror(errno));
	    mlfi->mlfi_wrkdir[0] = '\0';
            SMFI_SETREPLY_TEMPFAIL();
            return SMFIS_TEMPFAIL;
	}
	if (chmod(mlfi->mlfi_wrkdir, S_IRWXU|S_IRGRP|S_IXGRP) == -1) {
	    logqiderr(mlfi, __func__, LOG_ERR, "could not change mode of "
		"directory %s: %s", mlfi->mlfi_wrkdir, strerror(errno));
	    SMFI_SETREPLY_TEMPFAIL();
	    return SMFIS_TEMPFAIL;
	}
    }
    logqidmsg(mlfi, LOG_DEBUG, "create work directory %s", mlfi->mlfi_wrkdir);

    /* Open file to store this message */
    (void) snprintf(mlfi->mlfi_fname, sizeof(mlfi->mlfi_fname) - 1,
	"%s/email.txt", mlfi->mlfi_wrkdir);
    if ((mlfi->mlfi_fp = fopen(mlfi->mlfi_fname, "w+")) == NULL) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not create message file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }
    if (fchmod(fileno(mlfi->mlfi_fp), S_IRUSR|S_IWUSR|S_IRGRP) == -1) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not change mode of file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }
    logqidmsg(mlfi, LOG_DEBUG, "create message file %s", mlfi->mlfi_fname);

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_ENVRCPT - Handle the envelope RCPT command
** 
** mlfi_envrcpt() is called once per recipient, hence one or more times
** per message, immediately after mlfi_envfrom()
*/
sfsistat
mlfi_envrcpt(SMFICTX *ctx, char **envrcpt)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);
    struct	mlfiAddress *rcpt, *r;
    int		rcptlen;

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_INFO, "RCPT TO: %s",  *envrcpt);

    /* Store recipient address */
    rcptlen = strlen(*envrcpt);
    if ((rcpt = malloc(sizeof(*rcpt) + rcptlen)) == NULL) {
	logqiderr(mlfi, __func__, LOG_ALERT, "could not allocate memory");
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }
    (void) strlcpy(rcpt->q_paddr, *envrcpt, rcptlen + 1);
    rcpt->q_next = NULL;
    if (mlfi->mlfi_rcpt == NULL) {
	mlfi->mlfi_rcpt = rcpt;
    } else {
	r = mlfi->mlfi_rcpt;
	while (r->q_next != NULL) {
	    r = r->q_next;
	}
	r->q_next = rcpt;
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_HEADER - Handle a message header
** 
** mlfi_header() is called zero or more times between mlfi_envrcpt() and
** mlfi_eoh(), once per message header
*/
sfsistat
mlfi_header(SMFICTX *ctx, char *headerf, char *headerv)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_DEBUG, "HEADER: %s: %s", headerf, headerv);

    /* Write the header to the message file */
    /* amavisd_new require \n instead of \r\n at the end of header */
    (void) fprintf(mlfi->mlfi_fp, "%s: %s\n", headerf, headerv);
    if (ferror(mlfi->mlfi_fp)) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not write to message file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_EOH - Handle the end of message headers
** 
** mlfi_eoh() is called once after all headers have been sent and processed
*/
sfsistat
mlfi_eoh(SMFICTX *ctx)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_DEBUG, "END OF HEADERS");

    /* Write the blank line between the header and the body */
    /* XXX: amavisd_new require \n instead of \r\n at the end of line */
    (void) fprintf(mlfi->mlfi_fp, "\n");
    if (ferror(mlfi->mlfi_fp)) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not write to message file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_BODY - Handle a piece of a message's body
** 
** mlfi_body() is called zero or more times between mlfi_eoh() and mlfi_eom()
*/
sfsistat
mlfi_body(SMFICTX *ctx, unsigned char * bodyp, size_t bodylen)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_DEBUG, "body chunk: %ld", (long)bodylen);

    /* Write the body chunk to the message file */
    if (fwrite(bodyp, bodylen, 1, mlfi->mlfi_fp) < 1) {
	logqiderr(mlfi, __func__, LOG_ERR, "could not write to message file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_EOM - Handle the end of a message
** 
** mlfi_eom() is called once after all calls to mlfi_body()
** for a given message
*/
sfsistat
mlfi_eom(SMFICTX *ctx)
{
    int		sd, i;
    char       *idx, *header, *rcode, *xcode, *name, *value;
    sfsistat	rstat;
    struct	mlfiCtx *mlfi = MLFICTX(ctx);
    struct	mlfiAddress *rcpt;
    struct	sockaddr_un amavisd_sock;
    int		wait_counter;

    /* Check milter private data */
    MLFI_CHECK_CTX();

    logqidmsg(mlfi, LOG_INFO, "CONTENT CHECK");

    /* Close the message file */
    if (mlfi->mlfi_fp == NULL) {
	logqiderr(mlfi, __func__, LOG_ERR, "message file %s is not opened",
	    mlfi->mlfi_fname);
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }
    if (fclose(mlfi->mlfi_fp) == -1) {
	mlfi->mlfi_fp = NULL;
	logqiderr(mlfi, __func__, LOG_ERR, "could not close message file "
	    "%s: %s", mlfi->mlfi_fname, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }
    mlfi->mlfi_fp = NULL;
    logqidmsg(mlfi, LOG_DEBUG, "close message file %s", mlfi->mlfi_fname);

    /* Wait for amavisd connection */
    if (max_sem != NULL) {
	wait_counter = 0;
	while (sem_trywait(max_sem) == -1) {
	    if (errno != EAGAIN) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not lock amavisd "
		    "connections semaphore: %s", strerror(errno));
		SMFI_SETREPLY_TEMPFAIL();
		return SMFIS_TEMPFAIL;
	    }
	    if (!(wait_counter++ < max_wait)) {
		logqiderr(mlfi, __func__, LOG_WARNING, "amavisd connection "
		    "not available for %d sec, giving up", wait_counter);
		SMFI_SETREPLY_TEMPFAIL();
		return SMFIS_TEMPFAIL;
	    }
	    if (!(wait_counter % 60)) {
		logqidmsg(mlfi, LOG_DEBUG, "amavisd connection not available "
		    "for %d sec, triggering sendmail", wait_counter);
		if (smfi_progress(ctx) != MI_SUCCESS) {
		    logqiderr(mlfi, __func__, LOG_ERR, "could not notify MTA "
			"that an operation is still in progress");
		    SMFI_SETREPLY_TEMPFAIL();
		    return SMFIS_TEMPFAIL;
		}
	    } else {
		logqidmsg(mlfi, LOG_DEBUG, "amavisd connection not available "
		    "for %d sec, sleeping", wait_counter);
	    }
	    sleep(1);
	}
	sem_getvalue(max_sem, &i);
	mlfi->mlfi_max_sem_locked = 1;
	logqidmsg(mlfi, LOG_DEBUG, "got amavisd connection %d for %d sec",
	    max_conns - i, wait_counter);
	if (smfi_progress(ctx) != MI_SUCCESS) {
	    logqiderr(mlfi, __func__, LOG_ERR, "could not notify MTA that an "
		"operation is still in progress");
	    SMFI_SETREPLY_TEMPFAIL();
	    return SMFIS_TEMPFAIL;
	}
    }

    /* Connect to amavisd */
    if ((sd = amavisd_connect(&amavisd_sock)) == -1) {
	logqiderr(mlfi, __func__, LOG_CRIT, "could not connect to amavisd "
	    "socket %s: %s", amavisd_socket, strerror(errno));
	SMFI_SETREPLY_TEMPFAIL();
	return SMFIS_TEMPFAIL;
    }

    logqidmsg(mlfi, LOG_DEBUG, "AMAVISD REQUEST");

    /* Send email to amavisd */
    AMAVISD_REQUEST("request", "AM.PDP");
    if (mlfi->mlfi_qid != NULL) {
	AMAVISD_REQUEST("queue_id", mlfi->mlfi_qid);
    }
    AMAVISD_REQUEST("sender", mlfi->mlfi_from);
    rcpt = mlfi->mlfi_rcpt;
    while (rcpt != NULL) {
	AMAVISD_REQUEST("recipient", rcpt->q_paddr);
	rcpt = rcpt->q_next;
    }
    AMAVISD_REQUEST("tempdir", mlfi->mlfi_wrkdir);
    AMAVISD_REQUEST("tempdir_removed_by", "client");
    AMAVISD_REQUEST("mail_file", mlfi->mlfi_fname);
    AMAVISD_REQUEST("delivery_care_of", "client");
    AMAVISD_REQUEST("client_address", mlfi->mlfi_addr);
    if (mlfi->mlfi_hostname != NULL) {
	AMAVISD_REQUEST("client_name", mlfi->mlfi_hostname);
    }
    if (mlfi->mlfi_helo != NULL) {
	AMAVISD_REQUEST("helo_name", mlfi->mlfi_helo);
    }
    AMAVISD_REQUEST(NULL, NULL);

    logqidmsg(mlfi, LOG_DEBUG, "AMAVISD RESPONSE");

    /* Process response from amavisd */
    rstat = SMFIS_TEMPFAIL;
    while (AMAVISD_RESPONSE(sd) != -1) {
	name = mlfi->mlfi_amabuf;

	/* Last response */
	if (*name == '\0') {
	    (void) amavisd_close(sd);
	    return rstat;
	}

	/* Get name and value */
        value = name;
	AMAVISD_PARSE_RESPONSE(value, value, '=');

	/* AM.PDP protocol version */
	if (strcmp(name, "version_server") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    i = (int) strtol(value, &header, 10);
	    if (header != NULL && *header != '\0') {
		logqiderr(mlfi, __func__, LOG_ERR, "malformed line '%s=%s'",
		    name, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }
	    if (i > AMPDP_VERSION) {
		logqiderr(mlfi, __func__, LOG_CRIT, "unknown AM.PDP protocol "
		    "version %d", i);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Add recipient */
	} else if (strcmp(name, "addrcpt") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    if (smfi_addrcpt(ctx, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not add recipient %s",
		    value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Delete recipient */
	} else if (strcmp(name, "delrcpt") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    if (smfi_delrcpt(ctx, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not delete recipient "
		    "%s", value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Append header */
	} else if (strcmp(name, "addheader") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    AMAVISD_PARSE_RESPONSE(header, value, ' ');
	    if (smfi_insheader(ctx, INT_MAX, header, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not add header "
		    "%s: %s", header, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Insert header */
	} else if (strcmp(name, "insheader") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    AMAVISD_PARSE_RESPONSE(idx, value, ' ');
	    i = (int) strtol(idx, &header, 10);
	    if (header != NULL && *header != '\0') {
		logqiderr(mlfi, __func__, LOG_ERR, "malformed line '%s=%s %s'",
		    name, idx, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }
	    AMAVISD_PARSE_RESPONSE(header, value, ' ');
	    if (smfi_insheader(ctx, i, header, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not insert header "
		    "%s %s: %s", idx, header, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Change header */
	} else if (strcmp(name, "chgheader") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    AMAVISD_PARSE_RESPONSE(idx, value, ' ');
	    i = (int) strtol(idx, &header, 10);
	    if (header != NULL && *header != '\0') {
		logqiderr(mlfi, __func__, LOG_ERR, "malformed line '%s=%s %s'",
		    name, idx, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }
	    AMAVISD_PARSE_RESPONSE(header, value, ' ');
	    if (smfi_chgheader(ctx, header, i, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not change header "
		    "%s %s: %s", idx, header, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Delete header */
        } else if (strcmp(name, "delheader") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    AMAVISD_PARSE_RESPONSE(idx, value, ' ');
	    i = (int) strtol(idx, &header, 10);
	    if (header != NULL && *header != '\0') {
		logqiderr(mlfi, __func__, LOG_ERR, "malformed line '%s=%s %s'",
		    name, idx, value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }
	    if (smfi_chgheader(ctx, value, i, NULL) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not delete header "
		    "%s %s:", idx, header);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Quarantine message */
	} else if (strcmp(name, "quarantine") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    if (smfi_quarantine(ctx, value) != MI_SUCCESS) {
		logqiderr(mlfi, __func__, LOG_ERR, "could not quarantine "
		    "message (%s)", value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Set response code */
        } else if (strcmp(name, "return_value") == 0) {
	    logqidmsg(mlfi, LOG_INFO, "%s=%s", name, value);
	    if (strcmp(value, "continue") == 0) {
		rstat = SMFIS_CONTINUE;
	    } else if (strcmp(value, "accept") == 0) {
		rstat = SMFIS_ACCEPT;
	    } else if (strcmp(value, "reject") == 0) {
		rstat = SMFIS_REJECT;
	    } else if (strcmp(value, "discard") == 0) {
		rstat = SMFIS_DISCARD;
	    } else if (strcmp(value, "tempfail") == 0) {
		rstat = SMFIS_TEMPFAIL;
	    } else {
		logqiderr(mlfi, __func__, LOG_ERR, "unknown return value %s",
		    value);
		SMFI_SETREPLY_TEMPFAIL();
		(void) amavisd_close(sd);
		return SMFIS_TEMPFAIL;
	    }

	/* Set SMTP reply */
        } else if (strcmp(name, "setreply") == 0) {
	    AMAVISD_PARSE_RESPONSE(rcode, value, ' ');
	    AMAVISD_PARSE_RESPONSE(xcode, value, ' ');
	    if (*rcode != '4' && *rcode != '5') {
		/* smfi_setreply accept only 4xx and 5XX codes */
		logqidmsg(mlfi, LOG_DEBUG, "%s=%s %s %s", name, rcode, xcode,
		    value);
	    } else {
		logqidmsg(mlfi, LOG_NOTICE, "%s=%s %s %s", name, rcode, xcode,
		    value);
		if (smfi_setreply(ctx, rcode, xcode, value) != MI_SUCCESS) {
		    logqiderr(mlfi, __func__, LOG_ERR, "could not set reply "
			"%s %s %s", rcode, xcode, value);
		    SMFI_SETREPLY_TEMPFAIL();
		    (void) amavisd_close(sd);
		    return SMFIS_TEMPFAIL;
		}
	    }

	/* Exit code */
        } else if (strcmp(name, "exit_code") == 0) {
	    /* ignore legacy exit_code */
	    logqidmsg(mlfi, LOG_DEBUG, "%s=%s", name, value);

	/* Unknown response */
        } else {
	    logqiderr(mlfi, __func__, LOG_WARNING, "ignore unknown amavisd "
		"response %s=%s", name, value);
	}
    }

    /* Amavisd response fail */
    logqidmsg(mlfi, LOG_DEBUG, "amavisd response line %s", name);
    logqiderr(mlfi, __func__, LOG_ERR, "could not read from amavisd socket "
	"%s: %s", amavisd_socket, strerror(errno));
    SMFI_SETREPLY_TEMPFAIL();
    (void) amavisd_close(sd);
    return SMFIS_TEMPFAIL;
}


/*
** MLFI_ABORT - Handle the current message's being aborted
**
** mlfi_abort() must reclaim any resources allocated on a per-message
** basis, and must be tolerant of being called between any two
** message-oriented callbacks
*/
sfsistat
mlfi_abort(SMFICTX *ctx)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    if (mlfi == NULL) {
	logqiderr(mlfi, __func__, LOG_DEBUG, "context is not set");
	return SMFIS_CONTINUE;
    }

    logqidmsg(mlfi, LOG_NOTICE, "ABORT");

    /* Cleanup message data */
    mlfi_cleanup_message(mlfi);

    /* Continue processing */
    return SMFIS_CONTINUE;
}


/*
** MLFI_CLOSE - The current connection is being closed
** 
** mlfi_close() is always called once at the end of each connection
*/
sfsistat
mlfi_close(SMFICTX *ctx)
{
    struct	mlfiCtx *mlfi = MLFICTX(ctx);

    /* Check milter private data */
    if (mlfi == NULL) {
	logqiderr(mlfi, __func__, LOG_DEBUG, "context is not set");
	return SMFIS_CONTINUE;
    }

    /* Release private data */
    MLFI_CLEANUP(mlfi);
    if (smfi_setpriv(ctx, NULL) != MI_SUCCESS) {
	/* NOTE: smfi_setpriv return MI_FAILURE when ctx is NULL */
	/* logqiderr(mlfi, __func__, LOG_ERR, "could not release milter context"); */
    }

    logqidmsg(mlfi, LOG_INFO, "CLOSE");

    /* Continue processing */
    return SMFIS_CONTINUE;
}
