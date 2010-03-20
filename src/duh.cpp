#include <iostream>

#include "cmdline.hpp"

cmdline<
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  > c;



int main(int argc, char **argv)
{
c.names = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_name,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.shorts = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_short,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.longs = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_long,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.svals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_default,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.descs = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_desc,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.ivals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) 0,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) 1,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) 0,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  c.bvals = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) true,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) false,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) false,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };
  c.parsecmdline(argc, argv);
}