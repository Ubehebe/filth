#ifndef TIME_HPP
#define TIME_HPP

#include <time.h>

class Time
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
  Time(char const *fmt = RFC822) : fmt(fmt) {}
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

#endif // TIME_HPP
