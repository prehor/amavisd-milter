# amavisd-milter(8).

## NAME

    amavisd-milter — sendmail milter for amavisd-new

## SYNOPSIS

    amavisd-milter [-Bfhv] [-d debug-level] [-D delivery-care-of]
                   [-m max-conns] [-M max-wait] [-p pidfile] [-P]
                   [-q backlog] [-s socket] [-t timeout] [-S socket]
                   [-T timeout] [-w directory]

## DESCRIPTION

The amavisd-milter is a sendmail milter (mail filter) for amavisd&#8209;new
2.4.3 and above and sendmail 8.13 and above (limited support for 8.12 is
provided).

Instead of older amavis-milter helper program, full amavisd&#8209;new functionality
is available, including adding spam and virus information header
fields, modifying Subject, adding address extensions and removing certain
recipients from delivery while delivering the same message to the rest.

For more information you can visit amavisd-milter website:
* https://github.com/prehor/amavisd-milter

### Options

The options are as follows:

    -B      Uses {daemon_name} macro as a policy bank name (see POLICY BANKS
            below).

    -d debug-level
            Set the debug level to debug-level.  Debugging traces become more
            verbose as the debug level increases.  Maximum is 9.

    -D delivery-care-of
            Set AM.PDP request attribute delivery_care_of to client (default)
            or server.  When client method is used then amavisd-milter is
            responsible to forward the message to recipients. This method
            doesn't allow personalized header or body modification.

            When server method is used then amavisd-new is responsible to
            forward the message to recipients and can provide personalized
            header and body modification.  $forward_method in amavisd.conf
            must point to some place willing to accept mail without further
            checking in amavisd-new.

    -f      Run amavisd-milter in the foreground (i.e. do not daemonize).
            Print debug messages to the terminal.

    -h      Print help page and exit.

    -m max-conns
            Maximum concurrent amavisd connections (default 0 - unlimited
            number of connections).  It must agree with the $max_servers
            entry in amavisd.conf.

    -M max-wait
            Maximum wait for connection to amavisd in seconds (default 300 =
            5 minutes).  It must be less then sending MTA timeout for a
            response to the final "." that terminates a message on sending
            MTA.  sendmail has default value 1 hour, postfix 10 minutes and
            qmail 20 minutes.  We suggest to use less than 10 minutes.

    -p pidfile
            Use this pid file (default /var/amavis/amavisd-milter.pid).

    -P      When amavisd-new fails mail will be passed through unchecked.

    -q backlog
            Sets the incoming socket backlog used by listen(2).  If it is not
            set or set to zero, the operating system default is used.

    -s socket
            Communication socket between sendmail and amavisd-milter (default
            /var/amavis/amavisd-milter.sock).  The protocol spoken over this
            socket is MILTER (Mail FILTER).  It must agree with the
            INPUT_MAIL_FILTER entry in sendmail.mc

            The socket should be in "proto:address" format:
            - {unix|local}:/path/to/file - A named pipe.
            - inet:port@{hostname|ip-address} - An IPV4 socket.
            - inet6:port@{hostname|ip-address} - An IPV6 socket.

    -S socket
            Communication socket between amavisd-milter and amavisd-new
            (default /var/amavis/amavisd.sock).  The protocol spoken over
            this socket is AM.PDP (AMavis Policy Delegation Protocol).  It
            must agree with the $unix_socketname entry in amavisd.conf.

            The socket should be in "proto:address" format:
            - {unix|local}:/path/to/file - A named pipe.

    -t timeout
            sendmail connection timeout in seconds (default 600 = 10 minutes).
            It must agree with the INPUT_MAIL_FILTER entry in sendmail.mc and
            must be greater than or equal to the amavisd-new connection timeout.
            When you use other milters (especially time consuming), the timeout
            must be sufficient to process message in all milters.

    -T timeout
            amavisd-new connection timeout in seconds (default 600 = 10
            minutes).  This timeout must be sufficient for message processing
            in amavisd-new.  It's usually a good idea to adjust them to the
            same value as sendmail connection timeout.

    -v      Report the version number and exit.

    -w directory
            Set working directory (default /var/amavis).

### Limited support for sendmail 8.12

* `smfi_addheader()` is used instead of `smfi_insheader()` for `insheader`
  and `addheader` AM.PDP responses. This works well with amavisd&#8209;new
  2.4.3 or newer.
* `smfi_progress()` isn't called when amavisd-milter wait for amavisd&#8209;new
  communication socket.
* AM.PDP response quarantine isn't implemented.

## FILES
    /var/amavis/amavisd-milter.pid
            The default process-id file.

    /var/amavis/amavisd-milter.sock
            The default sendmail communication socket.

    /var/amavis/amavisd.sock
            The default amavisd-new communication socket.

    /var/amavis
            The default working directory.

## POLICY BANKS

If the option `-B` is enabled, amavisd-milter uses the value of the milter
macro {daemon_name} as a name of the amavisd&#8209;new policy bank. Usualy this
milter macro is set to name of the MTA.

When remote client is authenticated, amavisd-milter uses this information
as a name of the amavisd&#8209;new policy banks:

    SMTP_AUTH
            Indicate that the remote client is authenticated.

    SMTP_AUTH_<MECH>
            Remote client authentication mechanism.

    SMTP_AUTH_<MECH>_<BITS>
            The number of bits used for the key of the symmetric cipher when
            authentication mechanism use it.

## EXAMPLES

### Configuring amavisd-new

In amavisd.conf file change protocol and socket settings to:

    $protocol = "AM.PDP";                      # Use AM.PDP protocol
    $unix_socketname = "$MYHOME/amavisd.sock"; # Listen on Unix socket
    ### $inet_socket_port = 10024;             # Don't listen on TCP port

Then (re)start amavisd daemon.

### Configuring sendmail

To the sendmail.mc file add the following entries:

    define(`confMILTER_MACROS_CONNECT', confMILTER_MACROS_CONNECT`, {client_resolve}')
    define(`confMILTER_MACROS_ENVFROM', confMILTER_MACROS_ENVFROM`, r, b')
    INPUT_MAIL_FILTER(`amavisd-milter',
        `S=local:/var/amavis/amavisd-milter.sock,
        F=T, T=S:10m;R:10m;E:10m')

Then rebuild your sendmail.cf file, install it (usually to
/etc/mail/sendmail.cf) and (re)start sendmail daemon.

### Running amavisd-milter
This example assume that amavisd&#8209;new is running as user amavis.  It must
agree with the entry $daemon_user in amavisd.conf.

First create working directory:

    mkdir /var/amavis/tmp
    chmod 750 /var/amavis/tmp
    chown amavis /var/amavis/tmp

Then start amavisd-milter as non-priviledged user amavis:

    su - amavis -c "amavisd-milter -w /var/amavis/tmp"

### Limiting maximum concurrent connections to amavisd

To limit concurrent connections to 4 and fail after 10 minutes (10*60 secs) of
waiting run amavisd-milter with this options:

    su - amavis -c "amavisd-milter -w /var/amavis/tmp -m 4 -M 600"

### Troubleshooting

For troubleshooting run amavisd-milter on the foreground and set debug
level to appropriate level:

    su - amavis -c "amavisd-milter -w /var/amavis/tmp -f -d level"

where debug levels are:

    1       Not errors but unexpected states (connection abort etc).
    2       Main states in message processing.
    3       All amavisd-milter debug messages.
    4-9     Milter communication debugging (smfi_setdbg 1-6).

## SEE ALSO

* https://github.com/prehor/amavisd-milter
* https://www.ijs.si/software/amavisd/
* https://www.sendmail.org

## AUTHORS

This manual page was written by Petr Rehor <<rx@rx.cz>> and is based on
Jerzy Sakol <<jerzy.sakol@commgraf.pl>> initial work.

## BUGS

Issues can be reported by using GitHub at:
* https://github.com/prehor/amavisd-milter/issues

Full details on how to report issues can be found in the Contribution
Guidelines at:
* https://github.com/prehor/amavisd-milter/blob/master/CONTRIBUTING.md

Enhancements, requests and problem reports are welcome.

If you run into problems first check GitHub Issues before creating a new
one.  It's highly likely somebody has already come across the same problem
and it's been solved.
