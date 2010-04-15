#ifndef TIME_NR_HPP
#define TIME_NR_HPP

#include <string.h>
#include <time.h>

/** \brief Thin wrapper around useful time and date formatting functions.
 * \warning Not thread-safe (the "_nr" means "non-reentrant"). But my intent
 * is for each worker to have a separate Time_nr object.
 * \todo Support more format strings? */
class Time_nr
{
public:
  static char const *RFC822; //!< RFC 822 format string
  static size_t const bufsz = 1<<6;
private:
  time_t _t;
  struct tm _tm;
  char buf[bufsz];
  char const *fmt;
public:
  Time_nr(char const *fmt = RFC822) : fmt(fmt) {}
  /** \brief Print given time. */
  char const *print(time_t const &t)
  {
    strftime(buf, bufsz, fmt, gmtime_r(&t, &_tm));
    return static_cast<char const *>(buf);
  }
  /** \brief Print current time. */
  char const *print()
  {
    time(&_t);
    return print(_t);
  }
  /** \brief Convert given string to time_t.
   * \warning Returns 0 (i.e. January 1970) on failure, which may or may not
   * be reasonable. */
  time_t scan(char const *s)
  {
    struct tm tm;
    memset((void *)&tm, 0, sizeof(struct tm));
    if (strptime(s, fmt, &tm)==NULL)
      return 0;
    else
      return mktime(&tm);
  }
};

#endif // TIME_NR_HPP
