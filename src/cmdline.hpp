#ifndef CMDLINE_HPP
#define CMDLINE_HPP

template<int N> struct cmdline
{
  char const *progdesc;
  char const *names[N];
  char const *shorts[N];
  char const *longs[N];
  char const *svals[N];
  char const *descs[N];
  int ivals[N];
  bool bvals[N];

  void parsecmdline(int argc, char **argv);
  void unrecognized_opt(char *argv0, char *opt);
  void print_help(char *argv0);
  void atoi_opts();

};

// Ugh! This is a crappy pattern to avoid template errors in the linker.
#include "cmdline.cpp"

#endif // CMDLINE_HPP
