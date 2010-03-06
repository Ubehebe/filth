#ifndef HTTP_ENV_H
#define HTTP_ENV_H

#include <sys/types.h>

namespace HTTP_env
{
  enum flag
    {
#define DEFINE_ME(_name, _short, _long, _default, _desc) name,
#include "HTTP_env.def"
#undef DEFINE_ME
    };

  // These are all defined in HTTP_env.cpp
  extern size_t const num_flag;
  extern char const *flag_names[];
  extern char const *flag_shorts[];
  extern char const *flag_longs[];
  extern char const *flag_vals[];
  extern char const *flag_descs[];

  void parseflags(int argc, char **argv);
};


#endif // HTTP_ENV_H
