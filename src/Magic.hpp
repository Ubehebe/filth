#ifndef MAGIC_NR_HPP
#define MAGIC_NR_HPP

#include <magic.h>
#include <stdlib.h>

#include "Locks.hpp"
#include "logging.h"

/* Thin wrapper around libmagic.
 * TODO: The libmagic documentation I have seen does not mention whether
 * it is thread-safe, so I am assuming it is not and doing coarse locking.
 * If I find out otherwise, the lock can go away. */

class Magic
{
  magic_t m;
  Mutex l;
public:
  Magic(int flags=MAGIC_MIME_TYPE, char const *database=NULL)
  {
    if ((m = magic_open(flags))==NULL) {
      _LOG_FATAL("magic_open: %m");
      exit(1);
    }
    if (magic_load(m, database)==-1) {
      _LOG_FATAL("magic_load: %m");
      exit(1);
    }
  }
  ~Magic() { magic_close(m); }
  char const *operator()(char const *filename)
  {
    l.lock();
    char const *ans = magic_file(m, filename);
    l.unlock();
    return ans;
  }
  void setflags(int flags)
  {
    l.lock();
    if (magic_setflags(m, flags)==-1) {
      _LOG_FATAL("magic_setflags: %m");
      exit(1);
    }
    l.unlock();
  }

};

#endif // MAGIC_NR_HPP
