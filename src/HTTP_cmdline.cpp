#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HTTP_cmdline.hpp"

namespace HTTP_cmdline
{

  size_t const nopt = 
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) +1
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) +1
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
    ;

  char const *names[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_name,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_name,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *shorts[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_short,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_short,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *longs[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_long,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_long,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *svals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_default,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_default,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  char const *descs[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) #_desc,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) #_desc,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  int ivals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) 0,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) 1,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) 0,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  bool bvals[] = {
#define DEFINE_ME_BOOL(_name, _short, _long, _desc, _default) true,
#define DEFINE_ME_INT(_name, _short, _long, _desc, _default) false,
#define DEFINE_ME_STR(_name, _short, _long, _desc, _default) false,
#include "../data/HTTP_cmdline.def"
#undef DEFINE_ME_STR
#undef DEFINE_ME_INT
#undef DEFINE_ME_BOOL
  };

  void parsecmdline(int argc, char **argv)
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
	for (j=0; j<nopt; ++j) {
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
	if (j == nopt)
	  unrecognized_opt(argv[0], tmp);
      }
      // Short form of flag.
      else {
	for (j=0; j<nopt; ++j) {
	  if (tmp[1] == shorts[j][0]) {
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
	if (j == nopt)
	  unrecognized_opt(argv[0], tmp);
      }
    }
    atoi_opts();
  }

  void unrecognized_opt(char *argv0, char *option)
  {
    printf("%s: unrecognized option '%s'\n"
	   "Try `%s --help' for more information.\n",
	   argv0, option, argv0);
    exit(1);
  }

  void print_help(char *argv0)
  {
    printf("Usage: %s [OPTION]\nA basic web server.\n", argv0);
    /* Note that this may not actually print the default values
     * if the --help flag is not the first flag parsed. Oh well! */
    for (int i=0; i< nopt; ++i) {
      printf("-%s, --%s\t%s %s%s%s\n",
	     shorts[i], longs[i], descs[i],
	     ((bvals[i]) ? "" : "[default "),
	     ((bvals[i]) ? "" : svals[i]),
	     ((bvals[i]) ? "" : "]"));
    }
    exit(0);
  }

  void atoi_opts()
  {
    for (int i=0; i<nopt; ++i) {
      if (ivals[i]) {
	ivals[i] = atoi(svals[i]);
      }
      else if (bvals[i]) {
	if (strncmp(svals[i], "false", strlen("false"))==0)
	  bvals[i] = false;
      }
    }
  }
};
