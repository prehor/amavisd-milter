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
 * $Id: main.c,v 1.12 2006/10/07 11:13:26 reho Exp $
 */

#include "amavisd-milter.h"

#include <sys/socket.h>
#include <sysexits.h>


/*
** USAGEMSG - Print error message, program usage and then exit
*/
#define USAGEMSG(format, arg) \
{ \
    (void) fprintf(stderr, "%s: " format "\n", progname , arg); \
    usage(progname); \
    exit(EX_USAGE); \
}


/*
** CHECK_OPTARG - Check optarg value existence
*/
#define CHECK_OPTARG(optarg) \
{ \
    if (optarg == NULL || *optarg == '\0') { \
	USAGEMSG("option requires an argument -- %c", (char)c); \
    } \
}


/*
** GLOBAL VARIABLES
*/
int	daemonize = 1;
int	daemonized = 0;
int	debug_level = LOG_WARNING;
int	max_conns = 0;
int	max_wait = 5 * 60;
sem_t	max_sem_t;
sem_t  *max_sem = NULL;
char   *pid_file = "/var/amavis/" PACKAGE ".pid";
char   *mlfi_socket = "/var/amavis/" PACKAGE ".sock";
long	mlfi_timeout = 600;
char   *amavisd_socket = "/var/amavis/amavisd.sock";
long	amavisd_timeout = 600;
char   *work_dir = "/var/amavis";


/*
** USAGE - Print usage info
*/
static void
usage(const char *progname)
{
    (void) fprintf(stdout, "\nUsage: %s [OPTIONS]\n", progname);
    (void) fprintf(stdout, "Options are:\n\
	-d debug-level		Set debug level\n\
	-f			Run in the foreground\n\
	-h			Print this page\n\
	-m max-conns		Maximum amavisd connections \n\
	-M max-wait		Maximum wait for connection in seconds\n\
	-p pidfile		Use this pid file\n\
	-s socket		Milter communication socket\n\
	-S socket		Amavisd communication socket\n\
	-t timeout		Milter connection timeout in seconds\n\
	-T timeout		Amavisd connection timeout in seconds\n\
	-v			Report the version and exit\n\
	-w directory		Set the working directory\n\n");
}


/*
** VERSIONINFO - Print program version info
*/
static void
versioninfo(const char *progname)
{
    (void) fprintf(stdout, "%s %s\n", progname, VERSION);
}


/*
** MAIN - Main program loop
*/
int
main(int argc, char *argv[])
{
    static	const char *args = "d:fhm:M:p:s:S:t:T:vw:";

    int		c, rstat;
    char       *p, *progname;
    FILE       *fp;
    struct	stat st;
    mode_t	save_umask;
    struct	sockaddr_un unix_addr;

    /* Program name */
    p = strrchr(argv[0], '/');
    if (p == NULL) {
	progname = argv[0];
    } else {
	progname = p + 1;
    }

    /* Open syslog */
    openlog(progname, LOG_PID, LOG_MAIL);

    /* Process command line options */
    while ((c = getopt(argc, argv, args)) != EOF) {
	switch (c) {
	case 'd':		/* debug level */
	    CHECK_OPTARG(optarg);
	    debug_level = (int) strtol(optarg, &p, 10);
	    if (p != NULL && *p != '\0') {
		USAGEMSG("debug level is not valid number: %s", optarg);
	    }
	    if (debug_level < 0) {
		USAGEMSG("negative debug level: %d", debug_level);
	    }
	    debug_level += LOG_WARNING;
	    break;
	case 'f':		/* run in foreground */
	    daemonize = 0;
	    break;
	case '?':		/* options parsing error */
	    (void) fprintf(stderr, "\n");
	case 'h':		/* help */
	    usage(progname);
	    exit(EX_OK);
	    break;
	case 'm':		/* maximum amavisd connections */
	    max_conns = (int) strtol(optarg, &p, 10);
	    if (p != NULL && *p != '\0') {
		USAGEMSG("maximum amavisd connections is not valid number: %s",
		    optarg);
	    }
	    if (max_conns < 0) {
		USAGEMSG("negative maximum amavisd connections: %d",
		    max_conns);
	    }
	    break;
	case 'M':		/* maximum wait for connection */
	    max_wait = (int) strtol(optarg, &p, 10);
	    if (p != NULL && *p != '\0') {
		USAGEMSG("maximum wait for connection is not valid number: %s",
		    optarg);
	    }
	    if (max_wait < 0) {
		USAGEMSG("negative maximum wait for connection: %d",
		    max_wait);
	    }
	    break;
	case 'p':		/* pid file name */
	    CHECK_OPTARG(optarg);
	    pid_file = optarg;
	    break;
	case 's':		/* milter communication socket */
	    CHECK_OPTARG(optarg);
	    if (strlen(optarg) >= sizeof(unix_addr.sun_path) - 1) {
		USAGEMSG("milter communication socket name too long: %s",
		    optarg);
	    }
	    mlfi_socket = optarg;
	    break;
	case 't':		/* milter connection timeout */
	    CHECK_OPTARG(optarg);
	    mlfi_timeout = (int) strtol(optarg, &p, 10);
	    if (p != NULL && *p != '\0') {
		USAGEMSG("milter connection timeout is not valid number: %s",
		    optarg);
	    }
	    if (mlfi_timeout < 0) {
		USAGEMSG("negative milter connection timeout: %ld",
		    mlfi_timeout);
	    }
	    break;
	case 'v':		/* version info */
	    versioninfo(progname);
	    exit(EX_OK);
	    break;
	case 'w':		/* work directory */
	    CHECK_OPTARG(optarg);
	    work_dir = optarg;
	    break;
	case 'S':		/* amavisd communication socket */
	    CHECK_OPTARG(optarg);
	    if (strlen(optarg) >= sizeof(unix_addr.sun_path) - 1) {
		USAGEMSG("amavisd communication socket name too long: %s",
		    optarg);
	    }
	    amavisd_socket = optarg;
	    break;
	case 'T':		/* amavisd connection timeout */
	    CHECK_OPTARG(optarg);
	    amavisd_timeout = (int) strtol(optarg, &p, 10);
	    if (p != NULL && *p != '\0') {
		USAGEMSG("amavisd connection timeout is not valid number: %s",
		    optarg);
	    }
	    if (amavisd_timeout < 0) {
		USAGEMSG("negative amavisd connection timeout: %ld",
		    amavisd_timeout);
	    }
	    break;
	default:		/* unknown option */
	    USAGEMSG("illegal option -- %c", (char)c);
	    break;
	}
    }

    /* Create amavisd connections semaphore */
    if (max_conns > 0) {
	if (sem_init(&max_sem_t, 0, max_conns) == -1) {
	    logmsg(LOG_ERR, "%s: could not initialize amavisd connections "
		"semaphore: %s", progname, strerror(errno));
	    exit(EX_SOFTWARE);
	} else {
	    max_sem = &max_sem_t;
	}
    }

    /* Check permissions on work directory */
    /* TODO: traverse work directory path */
    if (stat(work_dir, &st) != 0) {
	logmsg(LOG_ERR, "%s: could not stat() to work directory %s: %s",
	    progname, work_dir, strerror(errno));
	exit(EX_SOFTWARE);
    }
    if (!S_ISDIR(st.st_mode)) {
	logmsg(LOG_ERR, "%s: %s is not directory", progname, work_dir);
	exit(EX_SOFTWARE);
    }
    if ((st.st_mode & S_IRWXO) != 0) {
	logmsg(LOG_ERR, "%s: work directory %s is world accessible", progname,
	    work_dir);
	exit(EX_SOFTWARE);
    }

    /* Configure milter */
    p = NULL;
    if (mlfi_socket[0] == '/') {
	p = mlfi_socket;
    }
    if (! strncmp(mlfi_socket, "unix:", 5)) {
	p = mlfi_socket + 5;
    }
    if (! strncmp(mlfi_socket, "local:", 6)) {
	p = mlfi_socket + 6;
    }
    if (p != NULL && unlink(p) != 0 && errno != ENOENT) {
	logmsg(LOG_ERR, "%s: could not unlink old milter socket %s: %s",
	    progname, mlfi_socket, strerror(errno));
	exit(EX_SOFTWARE);
    }
    if (debug_level > LOG_DEBUG &&
	smfi_setdbg(debug_level - LOG_DEBUG) != MI_SUCCESS)
    {
	logmsg(LOG_ERR, "%s: could not set milter debug level", progname);
	exit(EX_SOFTWARE);
    }
    if (mlfi_timeout > 0 && smfi_settimeout(mlfi_timeout) != MI_SUCCESS) {
	logmsg(LOG_ERR, "%s: could not set milter timeout", progname);
	exit(EX_SOFTWARE);
    }
    if (smfi_setconn(mlfi_socket) != MI_SUCCESS) {
	logmsg(LOG_ERR, "%s: could not set milter socket", progname);
	exit(EX_SOFTWARE);
    }
    if (smfi_register(smfilter) != MI_SUCCESS) {
	logmsg(LOG_ERR, "%s: could not register milter", progname);
	exit(EX_SOFTWARE);
    }

    /* Unlink old pid file */
    if (pid_file != NULL) {
	if (unlink(pid_file) != 0 && errno != ENOENT) {
	    logmsg(LOG_WARNING, "%s: could not unlink old pid file %s: %s",
		progname, pid_file, strerror(errno));
	}
    }

    /* Run in the background */
    if (daemonize) {
	if (daemon(1, 1) != -1) {
	    daemonized = 1;
	} else {
	    logmsg(LOG_ERR, "%s: could not fork daemon process: %s", progname,
		strerror(errno));
	    exit(EX_OSERR);
	}
    }

    /* Connect to milter socket */
    if (smfi_opensocket(false) != MI_SUCCESS) {
	logmsg(LOG_ERR, "could not open milter socket %s", mlfi_socket);
        exit(EX_SOFTWARE);
    }

    /* Greetings message */
    logmsg(LOG_WARNING, "starting %s %s on socket %s", progname, VERSION,
	mlfi_socket);

    /* Create pid file */
    if (pid_file != NULL) {
	save_umask = umask(022);
	fp = fopen(pid_file, "w");
	if (fp == NULL) {
	    logmsg(LOG_WARNING, "could not create pid file %s: %s", pid_file,
		strerror(errno));
	} else {
	    (void) fprintf(fp, "%ld\n", (long) getpid());
	    if (ferror(fp)) {
		logmsg(LOG_WARNING, "could not write to pid file %s: %s",
		    pid_file, strerror(errno));
		clearerr(fp);
		(void) fclose(fp);
	    } else if (fclose(fp) != 0) {
		logmsg(LOG_WARNING, "could not close pid file %s: %s", pid_file,
		    strerror(errno));
	    }
	}
	umask(save_umask);
    }

    /* Run milter */
    if ((rstat = smfi_main()) != MI_SUCCESS) {
	logmsg(LOG_ALERT, "%s failed", progname);
    } else {
	logmsg(LOG_WARNING, "stopping %s %s on socket %s", progname, VERSION,
	    mlfi_socket);
    }

    /* Unlink pid file */
    if (pid_file != NULL) {
	if (unlink(pid_file) != 0) {
	    logmsg(LOG_WARNING, "%s: could not unlink pid file %s: %s",
		progname, pid_file, strerror(errno));
	}
    }

    /* Destroy amavisd connections semaphore */
    if (max_sem != NULL && sem_destroy(max_sem) == -1) {
	logmsg(LOG_WARNING, "%s: could not destroy amavisd connections "
	    "semaphore: %s", progname, strerror(errno));
    }

    return rstat;
}
