#ifndef HTTP_CMDLINE_HPP
#define HTTP_CMDLINE_HPP

#include <sys/types.h>

namespace HTTP_cmdline
{
  enum flag
    {
#define DEFINE_ME(_name, _short, _long, _default, _desc) _name,
#include "HTTP_cmdline.def"
#undef DEFINE_ME
    };

  // These are all defined in HTTP_cmdline.cpp
  extern size_t const num_flag;
  extern char const *flag_names[];
  extern char const *flag_shorts[];
  extern char const *flag_longs[];
  extern char const *flag_vals[];
  extern char const *flag_descs[];

  void parsecmdline(int argc, char **argv);
  inline void unrecognized_option(char *argv0, char *option);
  inline void print_help(char *argv0);
};


#endif // HTTP_CMDLINE_HPP
