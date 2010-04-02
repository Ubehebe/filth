#include <stdlib.h>

#include "logging.h"
#include "Magic.hpp"

Magic::Magic(int flags, char const *database)
{
  /* TODO: libmagic has these weird error reporting functions,
   * magic_error, magic_errno that I think retrieve errno from inside
   * the library calls. Maybe they provide more accurate error reporting? */
  if ((m = magic_open(flags))==NULL) {
    _LOG_FATAL("magic_open: %m");
    exit(1);
  }
  if (magic_load(m, database)==-1) {
    _LOG_FATAL("magic_load: %m");
    exit(1);
  }
}

Magic::~Magic()
{
  // Just grab the lock so it will wait in case anyone else is using it.
  l.lock();
  magic_close(m);
  l.unlock();
}

void Magic::setflags(int flags)
{
  if (magic_setflags(m, flags)==-1) {
    _LOG_FATAL("magic_setflags: %m");
    exit(1);
  }
}

// returns NULL on error
char const *Magic::operator()(char const *filename)
{
  l.lock();
  char const *ans = magic_file(m, filename);
  l.unlock();
  return ans;
}
