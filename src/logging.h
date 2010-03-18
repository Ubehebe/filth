#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <syslog.h>

/* The system logger turns out to be incredibly expensive; in the past it
 * has increased my heap usage by an order of magnitude. Therefore to avoid
 * encouraging its use, I don't expose all the levels given in man 3 syslog.
 * _LOG_FATAL is for when the very next statement is exit().
 * _LOG_INFO is for a major change in state (e.g. cache flush) or anything
 * the user should see (e.g. the address of a listening socket).
 * _LOG_DEBUG is for anything else and should absolutely be turned off
 * for production use.
 * _LOG_FATAL and _LOG_INFO are also sent to stderr by default. */

/* K&R A12.3: "Unless the parameter in the replacement sequence is preceded
 * by #, or preceded or followed by ##, the argument tokens are examined for
 * macro calls, and expanded as necessary, just before insertion." =) */
#define QUOT_(x) #x
#define QUOT(x) QUOT_(x)
#define _SRC __FILE__"."QUOT(__LINE__)": "

#ifdef _LOG_DEBUG
#undef _LOG_DEBUG
#undef _LOG_INFO
#undef _LOG_FATAL
#define SYSLOG_OPTS LOG_PERROR
#define _LOG_DEBUG(...) syslog(LOG_USER|LOG_DEBUG, _SRC"debug: "__VA_ARGS__)
#define _LOG_INFO(...) syslog(LOG_USER|LOG_INFO, _SRC"info: "__VA_ARGS__)
#define _LOG_FATAL(...) syslog(LOG_USER|LOG_CRIT, _SRC"fatal: "__VA_ARGS__)
#warning debug logging enabled; expect the heap to be huge
#else // #ifdef _LOG_DEBUG
#ifdef _LOG_INFO
#define SYSLOG_OPTS 0
#define _LOG_DEBUG(...)
#undef _LOG_INFO
#undef _LOG_FATAL
#define _LOG_INFO(...) syslog(LOG_USER|LOG_INFO, _SRC"info: " __VA_ARGS__)
#define _LOG_FATAL(...) syslog(LOG_USER|LOG_CRIT, _SRC"fatal: " __VA_ARGS__)
#else // #ifdef _LOG_INFO
#ifdef _LOG_FATAL
#undef _LOG_FATAL
#define SYSLOG_OPTS 0
#define _LOG_DEBUG(...)
#define _LOG_INFO(...)
#define _LOG_FATAL(...) syslog(LOG_USER|LOG_CRIT, _SRC"fatal: " __VA_ARGS__)
#else // #ifdef _LOG_FATAL
#define SYSLOG_OPTS 0
#define _LOG_DEBUG(...)
#define _LOG_INFO(...)
#define _LOG_FATAL(...)
#endif // #ifdef _LOG_FATAL
#endif // #ifdef _LOG_INFO
#endif // #ifdef _LOG_DEBUG
#endif // LOGGING_H
