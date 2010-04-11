#ifndef MAGIC_NR_HPP
#define MAGIC_NR_HPP

#include <magic.h>
#include <stdlib.h>

#include "Factory.hpp"
#include "logging.h"

/* Thin wrapper around libmagic.
 * The libmagic documentation I have seen does not mention whether
 * it is thread-safe, so I am assuming it is not ("_nr" means "non-reentrant").
 * Besides, it probably makes sense for each worker to have one of these. */
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
  char const *operator()(char const *filename)
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

template<> class Factory<Magic_nr>
{
public:
  Magic_nr *operator()() { return new Magic_nr(); }
};

#endif // MAGIC_NR_HPP
