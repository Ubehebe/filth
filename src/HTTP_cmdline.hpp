#ifndef HTTP_CMDLINE_HPP
#define HTTP_CMDLINE_HPP

#include <sys/types.h>

/* Namespace to handle parsing command-line options.
 * I believe that using even more C preprocessor magic and maybe some
 * templates, I could have provided a way to specify command-line
 * arguments of any type. I'm not sure I've ever seen a command-line option
 * that's not one of those three, however, so I just kept it simple. */
namespace HTTP_cmdline
{
  enum opt
    {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) _name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) _name,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    };

  // These are all defined in HTTP_cmdline.cpp
  extern size_t const nopt;
  extern char const *names[];
  extern char const *shorts[];
  extern char const *longs[];
  extern char const *svals[];
  extern int ivals[];
  extern bool bvals[];
  extern char const *descs[];

  void parsecmdline(int argc, char **argv);
  inline void unrecognized_opt(char *argv0, char *opt);
  inline void print_help(char *argv0);
  inline void atoi_opts();
};


#endif // HTTP_CMDLINE_HPP
