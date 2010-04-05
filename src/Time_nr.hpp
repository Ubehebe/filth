#ifndef TIME_NR_HPP
#define TIME_NR_HPP

#include <time.h>

/* "_nr" means "non-reentrant". The intent is for each worker who needs
 * timestamps to have one of these. If the overhead is too much
 * (seems unlikely), it would be easy to wrap locks around one shared by
 * several threads. */
class Time_nr
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
  Time_nr(char const *fmt = RFC822) : fmt(fmt) {}
  char const *operator()(time_t const &t)
  {
    strftime(buf, bufsz, fmt, gmtime_r(&t, &_tm));
    return static_cast<char const *>(buf);
  }
  char const *operator()()
  {
    time(&_t);
    return (*this)(_t);
  }
};

#endif // TIME_NR_HPP
