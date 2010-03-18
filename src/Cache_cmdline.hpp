#ifndef CACHE_CMDLINE_HPP
#define CACHE_CMDLINE_HPP

#include "cmdline.hpp"

namespace Cache_cmdline
{
  enum {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) _name,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  extern cmdline<
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    > c;

  void cmdlinesetup(int argc, char **argv);
};

#endif // CACHE_CMDLINE_HPP
