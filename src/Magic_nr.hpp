#ifndef MAGIC_NR_HPP
#define MAGIC_NR_HPP

#include <magic.h>
#include <stdlib.h>

#include "logging.h"

/** \brief Thin wrapper around libmagic.
 * \warning The libmagic documentation I have seen does not mention whether
 * it is thread-safe, so I am assuming it is not ("_nr" means "non-reentrant").
 * Besides, it probably makes sense for each worker to have one of these.
 * \warning libmagic is not infallible! I have seen it give "text/x-c" for a CSS
 * file, for example. If the client knows the kind of file it wants, perhaps
 * it's best to trust the client. */
class Magic_nr
{
  magic_t m;
public:
  /** \brief Wrapper for magic_open and magic_load. */
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
  /** \brief Wrapper for magic_close. */
  ~Magic_nr() { magic_close(m); }
  /** \brief Wrapper for magic_file. */
  char const *operator()(char const *filename)
  {
    return magic_file(m, filename);
  }
  /** \brief Wrapper for magic_setflags. */
  void setflags(int flags)
  {
    if (magic_setflags(m, flags)==-1) {
      _LOG_FATAL("magic_setflags: %m");
      exit(1);
    }
  }
};

#endif // MAGIC_NR_HPP
