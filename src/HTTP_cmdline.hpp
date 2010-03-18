#ifndef HTTP_CMDLINE_HPP
#define HTTP_CMDLINE_HPP

#include "cmdline.hpp"

namespace HTTP_cmdline
{
  enum {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) _name,
#include "HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  extern cmdline<
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    > c;

  void cmdlinesetup(int argc, char **argv);
};

#endif // HTTP_CMDLINE_HPP
