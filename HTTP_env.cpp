#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HTTP_env.h"

namespace HTTP_env
{

  size_t const num_flag = 
#define DEFINE_ME(_name, _short, _long, _default, _desc) +1
#include "HTTP_env.def"
#undef DEFINE_ME
    ;

  char const *flag_names[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_name,
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_shorts[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_short,
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_longs[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_long,
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_vals[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_default,
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_descs[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_desc,
#include "HTTP_env.def"
#undef DEFINE_ME
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
	unrecognized_option(argv[0], tmp);
      }
      // Long form of flag.
      else if (tmp[1] == '-') {
	for (j=0; j<num_flag; ++j) {
	  if (strncmp(&tmp[2], flag_longs[j],
		      strlen(flag_longs[j]))==0) {
	    // Right now, all the options require args.
	    if ((eq = strchr(tmp, '='))==NULL)
	      unrecognized_option(argv[0], tmp);
	    flag_vals[j] = eq+1;
	    break;
	  }
	}
	// Didn't match a known option.
	if (j == num_flag)
	  unrecognized_option(argv[0], tmp);
      }
      // Short form of flag.
      else {
	for (j=0; j<num_flag; ++j) {
	  if (tmp[1] == flag_shorts[j][0]) {
	    // Option has not argument.
	    if (strlen(tmp) < 3)
	      unrecognized_option(argv[0], tmp);
	    flag_vals[j] = tmp+2;
	    break;
	  }
	}
	// No match found.
	if (j == num_flag)
	  unrecognized_option(argv[0], tmp);
      }
    }
  }

  void unrecognized_option(char *argv0, char *option)
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
    for (int i=0; i< num_flag; ++i)
      printf("-%s, --%s\t%s [default %s]\n",
	     flag_shorts[i], flag_longs[i], flag_descs[i], flag_vals[i]);
    exit(0);
  }
};
