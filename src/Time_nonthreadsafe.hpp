#ifndef TIME_NONTHREADSAFE_HPP
#define TIME_NONTHREADSAFE_HPP

#include <time.h>

/* The intent is for each thread who needs to take time to carry around
 * one of these. If the overhead is too much (seems unlikely), it would be
 * easy to wrap locks around one shared by several threads. */
class Time_nonthreadsafe
{
public:
  static char const *RFC822;
  static size_t const bufsz = 1<<6; // Need more flexibility?
private:
  time_t _t;
  struct tm _tm;
  char buf[bufsz];
  char const *fmt;
public:
  Time_nonthreadsafe(char const *fmt = RFC822) : fmt(fmt) {}
  time_t now() { return time(&_t); }
  char const *operator()()
  {
    time(&_t);
    strftime(buf, bufsz, fmt, gmtime_r(&_t, &_tm));
    return static_cast<char const *>(buf);
  }
};

#endif // TIME_NONTHREADSAFE_HPP
