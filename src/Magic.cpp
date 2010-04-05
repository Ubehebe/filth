#include <stdlib.h>

#include "logging.h"
#include "Magic_nr.hpp"

using namespace std;

Magic_nr::Magic_nr(int flags, char const *database)
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

Magic_nr::~Magic_nr()
{
  magic_close(m);
}

void Magic_nr::setflags(int flags)
{
  if (magic_setflags(m, flags)==-1) {
    _LOG_FATAL("magic_setflags: %m");
    exit(1);
  }
}

// returns NULL on error
char const *Magic_nr::operator()(char const *filename) const
{
  return magic_file(m, filename);
}
