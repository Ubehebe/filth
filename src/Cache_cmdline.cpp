#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Cache_cmdline.hpp"

namespace Cache_cmdline
{
  cmdline<
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    > c;

  void cmdlinesetup(int argc, char **argv)
  {
    c.progdesc = "A basic filesystem cache, communicating via domain sockets.";

    c.shorts = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_short,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

    c.longs = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_long,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

    c.svals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_default,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

    c.descs = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_desc,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

    c.ivals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) 0,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) 1,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) 0,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

    c.bvals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) true,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) false,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) false,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };
    c.parsecmdline(argc, argv);
  }
};
