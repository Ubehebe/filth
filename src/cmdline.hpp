#ifndef CMDLINE_HPP
#define CMDLINE_HPP

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** \brief Define and parse command-line options.
 * \note This class is a little awkward since we want to define command-line
 * options in a .def file included by the preprocessor. For example, in foo.def
 * we put
 * \code
 * DEFINE_ME_STR(mount, m, mount, where in the filesystem to mount, /)
 * DEFINE_ME_INT(workers, w, workers, number of worker threads, 10)
 * \endcode
 * Then in foo_cmdline.hpp we put:
 * \code
 * namespace foo_cmdline {
 * enum {
 * #define DEFINE_ME_STR(_name, _short, _long, _desc, _default) _name,
 * #define DEFINE_ME_INT(_name, _short, _long, _desc, _default) _name,
 * #include "foo.def"
 * #undef DEFINE_ME_INT
 * #undef DEFINE_ME_STR
 * };
 *
 * extern cmdline<
 * #define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
 * #define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
 * #include "foo.def"
 * #undef DEFINE_ME_INT
 * #undef DEFINE_ME_STR
 * > c;
 * };
 * \endcode
 * Then in foo_cmdline.cpp we put:
 * \code
 * namespace foo_cmdline {
 * c.shorts = {
 * #define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_short,
 * #define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_short,
 * #include "foo.def"
 * #undef DEFINE_ME_STR
 * #undef DEFINE_ME_INT
 * };
 * // Request that DEFINE_ME_INTs be turned into ints 
 * c.ivals = {
 * #define DEFINE_ME_INT(_name, _short, _long, _desc, _default) 1,
 * #define DEFINE_ME_STR(_name, _short, _long, _desc, _default) 0,
 * #include "foo.def"
 * #undef DEFINE_ME_STR
 * #undef DEFINE_ME_INT
 * };
 * // etc.
 * };
 * \endcode */
template<int N> struct cmdline
{
  char const *progdesc; //!< one-sentence description at beginning of --help
  char const *shorts[N]; //!< short form of option flag, e.g. -c (- for nothing)
  char const *longs[N]; //!< long form of option flag, e.g. --cache
  char const *descs[N]; //!< description of argument given in --help
  char const *svals[N]; //!< string value of argument

  int ivals[N]; //!< integer value of argument, if applicable
  bool bvals[N]; //!< boolean value of argument, if applicable

  /** \brief Turns strings into corresponding integers or booleans. */
  void atoi_opts();
  /** \brief Parse the command-line arguments. */
  void parsecmdline(int argc, char **argv);
  /** \brief Responsible for --help documentation. */
  void print_help(char *argv0);
  /** \brief Returns the number of the signal with canonical name sig. */
  int sigconv(char const *sig);
  /** \brief Complains to user and exits the process. */
  void unrecognized_opt(char *argv0, char *opt);
};

template<int N> int cmdline<N>::sigconv(char const *sig)
{
  static int nsig =
#define DEFINE_ME(x) +1
#include "sigs.def"
#undef DEFINE_ME
    ;

  static int signos[] = {
#define DEFINE_ME(x) SIG##x,
#include "sigs.def"
#undef DEFINE_ME
  };

  static char const *sigstrs[] = {
#define DEFINE_ME(x) #x,
#include "sigs.def"
#undef DEFINE_ME
  };
  
  for (int i=0; i < nsig; ++i)
    if (strncmp(sig, sigstrs[i], strlen(sigstrs[i]))==0)
      return signos[i];
  
  printf("unsupported signal %s\n", sig);
  exit(0);
}

template<int N> void cmdline<N>::parsecmdline(int argc, char **argv)
{
  char *tmp, *eq;
  char const *helpme = "--help";

  int i,j;
  for (i=1; i<argc; ++i) {
    tmp = argv[i];
    if (strncmp(tmp, helpme, strlen(helpme))==0) {
      print_help(argv[0]);
    }
    // Malformed option.
    if (tmp[0] != '-' || strlen(tmp) == 1) {
      unrecognized_opt(argv[0], tmp);
    }
    // Long form of flag.
    else if (tmp[1] == '-') {
      for (j=0; j<N; ++j) {
	if (strncmp(&tmp[2], longs[j],
		    strlen(longs[j]))==0) {
	  // Only boolean options can take no arguments.
	  if ((eq = strchr(tmp, '='))==NULL) {
	    if (bvals[j])
	      svals[j] = "true";
	    else
	      unrecognized_opt(argv[0], tmp);
	  }
	  else {
	    svals[j] = eq+1;
	  }
	  break;
	}
      }
      // Didn't match a known option.
      if (j == N)
	unrecognized_opt(argv[0], tmp);
    }
    // Short form of flag.
    else {
      for (j=0; j<N; ++j) {
	if (tmp[1] == shorts[j][0] && tmp[1] != '-') {
	  // Only boolean options can take no argument.
	  if (strlen(tmp) < 3) {
	    if (bvals[j])
	      svals[j] = "true";
	    else
	      unrecognized_opt(argv[0], tmp);
	  }
	  else {
	    svals[j] = tmp+2;
	  }
	  break;
	}
      }
      // No match found.
      if (j == N)
	unrecognized_opt(argv[0], tmp);
    }
  }
  atoi_opts();
}

template<int N> void cmdline<N>::unrecognized_opt(char *argv0, char *option)
{
  printf("%s: unrecognized option '%s'\n"
	 "Try `%s --help' for more information.\n",
	 argv0, option, argv0);
  exit(1);
}

template<int N> void cmdline<N>::print_help(char *argv0)
{
  printf("Usage: %s [options]\n%s\n", argv0, progdesc);
  /* Note that this may not actually print the default values
   * if the --help flag is not the first flag parsed. Oh well! */
  for (int i=0; i<N; ++i) {
    printf("%s%s%s--%s\t%s %s%s%s\n",
	   ((shorts[i][0] == '-') ? "" : "-"),
	   ((shorts[i][0] == '-') ?  "" : shorts[i]),
	   ((shorts[i][0] == '-') ? "" : ", "),
	   longs[i],
	   descs[i],
	   ((bvals[i]) ? "" : "[default "),
	   ((bvals[i]) ? "" : svals[i]),
	   ((bvals[i]) ? "" : "]"));
  }
  exit(0);
}

template<int N> void cmdline<N>::atoi_opts()
{
  for (int i=0; i<N; ++i) {
    if (ivals[i]) {
      ivals[i] = atoi(svals[i]);
    }
    else if (bvals[i]) {
      if (strncmp(svals[i], "false", strlen("false"))==0)
	bvals[i] = false;
    }
  }
}

#endif // CMDLINE_HPP
