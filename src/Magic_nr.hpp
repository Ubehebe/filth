#ifndef MAGIC_NR_HPP
#define MAGIC_NR_HPP

#include <magic.h>
#include <stdlib.h>

#include "logging.h"

/* Thin wrapper around libmagic. "_nr" means "non-reentrant"; the intent
 * is for each worker thread to have one of these to do lookups with.
 * Could easily be reentrant with locks. */

class Magic_nr
{
  magic_t m;
public:
  Magic_nr(int flags=MAGIC_MIME_TYPE, char const *database=NULL)
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
  ~Magic_nr() { magic_close(m); }
  char const *operator()(char const *filename) const
  {
    return magic_file(m, filename);
  }
  void setflags(int flags)
  {
    if (magic_setflags(m, flags)==-1) {
      _LOG_FATAL("magic_setflags: %m");
      exit(1);
    }
  }

};

#endif // MAGIC_NR_HPP
