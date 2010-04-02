#ifndef MAGIC_HPP
#define MAGIC_HPP

#include <magic.h>

#include "Locks.hpp"

/* Thin wrapper around libmagic. */

class Magic
{
  /* The libmagic man pages don't say whether queries are thread-safe, so
   * we just lock the magic cookie object. */
  Mutex l;
  magic_t m;
public:
  Magic(int flags=MAGIC_MIME_TYPE, char const *database=NULL);
  ~Magic();
  char const *operator()(char const *filename);
  void setflags(int flags);
};

#endif // MAGIC_HPP
