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
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_short
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_longs[] = {
#define DEFINE_ME(_name, _short, _long, _default, _desc) #_long
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_vals[] = {
#define DEFINE_ME(name, short, long, _default, _desc) #_default
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  char const *flag_descs[] = {
#define DEFINE_ME(name, short, long, _default, _desc) #_desc
#include "HTTP_env.def"
#undef DEFINE_ME
  };

  void parseflags(int argc, char **argv)
  {
    char *tmp, *eq;
    int i,j;
    for (i=1; i<argc; ++i) {
      tmp = argv[i];
      if (tmp[0] != '-' || strlen(tmp) == 1) {
	printf("%s: extra operand `%s'\n"
	       "Try `%s --help' for more information.\n",
	       argv[0], tmp, argv[0]);
	exit(1);
      }
      // Long form of flag.
      else if (tmp[1] == '-') {
	for (j=0; j<num_flag; ++j) {
	  if (strncmp(&tmp[2], flag_longs[j],
		      strlen(flag_longs[j]))==0) {
	    // Right now, all the options require args.
	    if ((eq = strchr(tmp, '='))==NULL) {
	      printf(
	      exit(1);
	    }
	    flag_vals[j] = eq+1;
	    break;
	  }
	}
	if (j == num_flag) {
	  printf("%s: unrecognized option '%s'\n"
		 "Try `%s --help' for more information.",
		 argv[0], tmp, argv[0]);
	  exit(1);
	}
      }
      // Short form of flag.
      else {
	for (j=0; j<num_flag; ++j) {
	  if (tmp[1] == flag_shorts[j][0]) {
	    flag_vals[j] = tmp+2;
	    break;
	  }
	}
	if (j == num_flag) {
	  printf("%s: invalid option -- '%c'\n"
		 "Try `%s --help' for more information.",
		 argv[0], tmp[1], argv[0]);
	  exit(1);
	}
      }
    }
  }

  void
};
