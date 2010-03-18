#include "Cache_cmdline.hpp"

namespace Cache_cmdline
{
  size_t const nopt = 
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    ;

  char const *names[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_name,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *shorts[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_short,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *longs[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_long,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *svals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_default,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *descs[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_desc,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  int ivals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) 0,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) 1,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) 0,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  bool bvals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) true,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) false,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) false,
#include "Cache_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };
};
