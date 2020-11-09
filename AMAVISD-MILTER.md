# amavisd-milter(8)

## NAME

**amavisd-milter** - sendmail milter for amavis

## SYNOPSIS

**amavisd-milter**
  [**-Bfhv**]
  [**-d**&nbsp;*debug-level*]
  [**-D**&nbsp;*delivery-care-of*]
  [**-m**&nbsp;*max-conns*]
  [**-M**&nbsp;*max-wait*]
  [**-p**&nbsp;*pidfile*]
  [**-P**]
  [**-q**&nbsp;*backlog*]
  [**-s**&nbsp;*socket*]
  [**-t**&nbsp;*timeout*]
  [**-S**&nbsp;*socket*]
  [**-T**&nbsp;*timeout*]
  [**-w**&nbsp;*directory*]

## DESCRIPTION

The amavisd-milter is a sendmail milter (mail filter) for amavis 2.4.3
and above and sendmail 8.13 and above.

With the amavisd-milter, a full amavis functionality is available,
including adding spam and virus information header fields, modifying
the Subject, adding address extensions and removing certain recipients
from delivery, while delivering the same message to the rest.

For more information you can visit amavisd-milter website:

> https://github.com/prehor/amavisd-milter

### Options

The options are as follows:

**-B**
: Uses the milter macro *{daemon_name}* as the policy bank name
  (see [POLICY BANKS](#policy-banks) below).

**-d** *debug-level*
: Set the debug level. The debugging traces become more detailed as the debug
  level increases. Maximum is 9.

**-D** *delivery-care-of*
: Set AM.PDP request attribute *delivery_care_of* to *client* (default) or
  *server*.

  When the *client* method is used, then amavisd-milter is responsible for
  forwarding the message to the recipients. This method does not allow
  personalized header or body modification.

  When the *server* method is used, then amavis is responsible for forwarding
  the message to the recipients and may personalize the headers and the body
  of the messages. *$forward_method* variable in **amavisd.conf** must point to
  a place willing to accept the message without further checking in amavis.

**-f**
: Run amavisd-milter in the foreground (i.e. do not daemonize).
  Print debugging messages to the terminal.

**-h**
: Print the help page and exit.

**-m** *max-conns*
: Maximum concurrent amavis connections (default 0 = unlimited number of
  connections). It must be the same as the *$max_servers* variable in
  **amavisd.conf**.

**-M** *timeout*
: Timeout for message processing in seconds (default 300 seconds = 5 minutes).
  Must be less then timeout for a response to the final "." that terminates a
  message on sending MTA. **Sendmail** uses default value 1 hour, **postfix**
  10 minutes and **qmail** 20 minutes. Recommended value is less than 10 minutes.

  If you use other milters (especially time-consuming), the timeout
  must be sufficient to process message in all milters.

**-p** *pidfile*
: Use this pid file.

**-P**
: When the amavis fails, the message will be passed through unchecked.

**-q** *backlog*
: Sets the incoming socket backlog used by **listen(2)**. If it is not set or
  set to zero, the operating system default is used.

**-s** *socket*
: Communication socket between sendmail and amavisd-milter. The protocol spoken
  over this socket is *MILTER* (Mail FILTER). It must have the same vale as the
  *INPUT_MAIL_FILTER* macro in **sendmail.mc**.

  The *socket* must be in format *proto:address*:

>  * *{unix|local}:/path/to/file* - A named pipe.
>  * *inet:port@{hostname|ip-address}* - An IPV4 socket.
>  * *inet6:port@{hostname|ip-address}* - An IPV6 socket.

**-S** *socket*
: Communication socket between amavisd-milter and amavis. The protocol spoken
  over this socket is *AM.PDP* (AMavis Policy Delegation Protocol). It must have
  the same value as the *$unix_socketname* variable in **amavisd.conf**.

  The *socket* must be in format *proto:address*:

>  * *{unix|local}:/path/to/file* - A named pipe.
>  * *inet:port@{hostname|ip-address}* - An IPV4 socket.
>  * *inet6:port@{hostname|ip-address}* - An IPV6 socket.

**-t** *timeout*
: Sendmail connection timeout in seconds (default 600 = 10 minutes).
  It must have the same vale as the *INPUT_MAIL_FILTER* macro in
  **sendmail.mc** and must be greater than or equal to the amavis connection
  timeout.

  If you use other milters (especially time-consuming), the timeout
  must be sufficient to process message in all milters.

**-T** *timeout*
: Amavis connection timeout in seconds (default 600 = 10 minutes).
  Must be sufficient to process message in amavis. Usually, it is a good idea
  to set them to the same value as sendmail connection timeout.

**-v**
: Report the version number and exit.

**-w** *directory*
: Set working directory.

## POLICY BANKS

If the option **-B** is enabled, amavisd-milter uses the value of the milter
macro *{daemon_name}* as the name of the amavis policy bank. Usually, this milter
macro is set to name of the MTA.

When remote client is authenticated, amavisd-milter uses authentication
information as the name of the amavis policy banks:

**`SMTP_AUTH`**
: Remote client has been authenticated.

**`SMTP_AUTH_<MECH>`**
: The remote client authentication mechanism.

**`SMTP_AUTH_<MECH>_<BITS>`**
: The number of bits used for the key of the symmetric cipher when
  authentication mechanism uses it.

## EXAMPLES

### Configuring amavis

In the **amavisd.conf** file set protocol and amavis socket to:

    $protocol = "AM.PDP";                      # Use AM.PDP protocol
    $unix_socketname = "$MYHOME/amavisd.sock"; # Listen on Unix socket
    ### $inet_socket_port = 10024;             # Do not listen on TCP port

Then (re)start the amavisd daemon.

### Configuring Postfix

Add the following entries to Postfix **main.cf***:

    smtpd_milters = local:<AMAVISD_MILTER.SOCK>
    milter_connect_macros = j {client_name} {daemon_name} v
    milter_protocol = 6

Then (re)start the Postfix daemon.

### Configuring sendmail

Add the following entries to file **sendmail.mc**:

    define(`confMILTER_MACROS_CONNECT',
      confMILTER_MACROS_CONNECT`, {client_resolve}')
    define(`confMILTER_MACROS_ENVFROM',
      confMILTER_MACROS_ENVFROM`, r, b')
    INPUT_MAIL_FILTER(`amavisd-milter',
      `S=local:<AMAVISD_MILTER.SOCK>, F=T, T=S:10m;R:10m;E:10m')

Then rebuild **sendmail.cf** file, install it and (re)start the sendmail daemon.

### Running amavisd-milter

This examples assumes that amavis is running as user *vscan*.
The actual name is shown in the *$daemon_user* variable in **amavisd.conf**.

### Limiting maximum concurrent connections to amavisd

To limit the maximum concurrent connections to amavis, run amavisd-milter with
this options:

    su - vscan -c "amavisd-milter -m 4"

### Troubleshooting

For troubleshooting, run amavisd-milter on the foreground and set the debug
level to the appropriate value:

    su - vscan -c "amavisd-milter -f -d 4"

Debug levels are:

* 1 - Not errors but unexpected states (connection abort etc).
* 2 - Main states in message processing.
* 3 - All amavisd-milter debug messages.
* 4-9 - Milter communication debugging (set **smfi_setdbg** to 1-6).

## SEE ALSO

* https://github.com/prehor/amavisd-milter
* https://www.ijs.si/software/amavisd/
* https://www.sendmail.org

## AUTHORS

This manual page was written by Petr Rehor and is based on Jerzy Sakol
initial work.

## BUGS

Issues can be reported by using GitHub at:

> https://github.com/prehor/amavisd-milter/issues

Full detailed information on how to report issues, please see the Contribution
Guidelines at:

> https://github.com/prehor/amavisd-milter/blob/master/CONTRIBUTING.md

Enhancements, requests and problem reports are welcome.

If you run into problems, first check the GitHub issues before creating a new
one. It is very likely that someone has encountered the same problem and it has
already been solved.
