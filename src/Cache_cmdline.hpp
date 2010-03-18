#ifndef CACHE_CMDLINE_HPP
#define CACHE_CMDLINE_HPP

#include <sys/types.h>

namespace Cache_cmdline
{
  enum opt
    {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) _name,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };
};

  // These are all defined in Cache_cmdline.cpp
  extern size_t const nopt;
  extern char const *names[];
  extern char const *shorts[];
  extern char const *longs[];
  extern char const *svals[];
  extern int ivals[];
  extern bool bvals[];
  extern char const *descs[];

#endif // CACHE_CMDLINE_HPP
