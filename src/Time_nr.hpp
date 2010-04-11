#ifndef TIME_NR_HPP
#define TIME_NR_HPP

#include <string.h>
#include <time.h>

#include "Factory.hpp"

/* The "_nr" means "non-reentrant"; the print function returns a pointer
 * to the object's internal buffer without any synchronization, clearly
 * a problem if multiple threads try to access it. But my intent is for each
 * worker to have a separate Time_nr object. */
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
  char const *print(time_t const &t)
  {
    strftime(buf, bufsz, fmt, gmtime_r(&t, &_tm));
    return static_cast<char const *>(buf);
  }
  char const *print()
  {
    time(&_t);
    return print(_t);
  }
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

template<> class Factory<Time_nr>
{
public:
  Time_nr *operator()() { return new Time_nr(); }
};

#endif // TIME_NR_HPP
