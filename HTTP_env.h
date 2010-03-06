#ifndef HTTP_ENV_H
#define HTTP_ENV_H

#include <sys/types.h>

namespace HTTP_env
{
  enum env
    {
#define DEFINE_ME_CONSTANT(name) name,
#define DEFINE_ME_PATH(name) name,
#include "HTTP_env.def"
#undef DEFINE_ME_PATH
#undef DEFINE_ME_CONSTANT
    };

  // These are all defined in HTTP_env.cpp
  extern size_t const num_env;
  extern char const *env_names[];
  extern char const *env_vals[];
  extern char const *progname;
  extern bool const env_ispath[];

  void collectenvs(char const *argv0);
};


#endif // HTTP_ENV_H
