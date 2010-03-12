#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <syslog.h>

/* Here is a list of levels accepted by syslog(), the description in man 3
 * syslog, and how I intend to use them. This header currently doesn't
 * support the two highest levels, LOG_EMERG ("system is unusable")
 * and LOG_ALERT ("action must be taken immediately").
 *
 * LOG_CRIT
 * "critical conditions".
 * My interpretation: the program will terminate immediately after logging.
 *
 * LOG_ERR
 * "error conditions".
 * My interpretation: an error that will cause a major change in program state
 * (e.g. if the kernel refuses an allocation, maybe we flush a cache).
 *
 * LOG_WARNING
 * "warning conditions".
 * My interpretation: non-normal but non-error conditions (e.g. a 
 * requested file cannot be found or open).
 *
 * LOG_NOTICE
 * "normal, but significant, condition".
 * My interpretation: Normal conditions that the user would like to see on
 * standard error (e.g. the address and port a server is listening on).
 *
 * LOG_INFO
 * "informational message".
 * My interpretation: tracks high-level change in the program's state (e.g.
 * a thread started or stopped, a major constructor/destructor was called).
 *
 * LOG_DEBUG
 * "debug-level message".
 * My interpretation: anything you want to print.
 */

/* The intent is to define the lowest-priority level of logging; every higher
 * priority will be automatically defined. The tokens are prepended with _
 * to prevent clashing with the corresponding macros in syslog.h. */

#ifdef _LOG_DEBUG
#define _LOG_INFO
#define _LOG_NOTICE
#define _LOG_WARNING
#define _LOG_ERR
#define _LOG_CRIT
#endif

#ifdef _LOG_INFO
#define _LOG_NOTICE
#define _LOG_WARNING
#define _LOG_ERR
#define _LOG_CRIT
#endif

#ifdef _LOG_NOTICE
#define _LOG_WARNING
#define _LOG_ERR
#define _LOG_CRIT
#endif

#ifdef _LOG_WARNING
#define _LOG_ERR
#define _LOG_CRIT
#endif

#ifdef _LOG_ERR
#define _LOG_CRIT
#endif

#ifdef _LOG_CRIT
#undef _LOG_CRIT
#define _LOG_CRIT(...) fprintf(stderr, __VA_ARGS__);	\
  syslog(LOG_USER|LOG_CRIT, __VA_ARGS__)
#else
#define _LOG_CRIT(...)
#endif

#ifdef _LOG_ERR
#undef _LOG_ERR
#define _LOG_ERR(...) syslog(LOG_USER|LOG_ERR, __VA_ARGS__)
#else
#define _LOG_ERR(...)
#endif

#ifdef _LOG_WARNING
#undef _LOG_WARNING
#define _LOG_WARNING(...) syslog(LOG_USER|LOG_WARNING, __VA_ARGS__)
#else
#define _LOG_WARNING(...)
#endif

#ifdef _LOG_NOTICE
#undef _LOG_NOTICE
#define _LOG_NOTICE(...) fprintf(stderr, __VA_ARGS__); \
  syslog(LOG_USER|LOG_NOTICE, __VA_ARGS__)
#else
#define _LOG_NOTICE(...)
#endif

#ifdef _LOG_INFO
#undef _LOG_INFO
#define _LOG_INFO(...) syslog(LOG_USER|LOG_INFO, __VA_ARGS__)
#else
#define _LOG_INFO(...)
#endif

#ifdef _LOG_DEBUG
#undef _LOG_DEBUG
#define _LOG_DEBUG(...) syslog(LOG_USER|LOG_DEBUG, __VA_ARGS__)
#else
#define _LOG_DEBUG(...)
#endif

#endif // LOGGING_H
