#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "HTTP_env.h"

namespace HTTP_env
{


  size_t const num_env = 
#define DEFINE_ME_CONSTANT(ignore) +1
#define DEFINE_ME_PATH(ignore) +1
#include "HTTP_env.def"
#undef DEFINE_ME_PATH
#undef DEFINE_ME_CONSTANT
			      ;

  char const *env_names[] = {
#define DEFINE_ME_CONSTANT(name) #name,
#define DEFINE_ME_PATH(name) #name,
#include "HTTP_env.def"
#undef DEFINE_ME_PATH
#undef DEFINE_ME_CONSTANT
  };

  /* Will all be filled in from environment vars at startup.
   * Note that some may need to be converted from strings to
   * numbers. */
  char const *env_vals[] = {
#define DEFINE_ME_CONSTANT(name) "",
#define DEFINE_ME_PATH(name) "",
#include "HTTP_env.def"
#undef DEFINE_ME_PATH
#undef DEFINE_ME_CONSTANT
  };

  // Will be filled in from argv[0] at startup
  char const *progname = "";

  bool const env_ispath[] = {
#define DEFINE_ME_CONSTANT(ignore) false,
#define DEFINE_ME_PATH(ignore) true,
#include "HTTP_env.def"
#undef DEFINE_ME_PATH
#undef DEFINE_ME_CONSTANT
  };

  /* Collect environment variables.
   * Every environment variable is of the form progname_xyzzy,
   * where progname is argv[0] (without the leading ./) and xyzzy
   * is a name defined in HTTP_env.def. */
  void collectenvs(char const *argv0)
  {
    progname = &argv0[2];
    std::string tmp;
    bool doabort = false;
  
    for (int i=0; i < num_env; ++i) {
      tmp = std::string(progname) + "_" + env_names[i];
      if ((env_vals[i] = getenv(tmp.c_str())) == NULL) {
	std::cout << "fatal error: environment variable "
		  << tmp << " undefined\n";
	doabort = true;
      }
      /* Some of the environment variables are filepaths. Make sure we can
       * visit these. */
      else if (env_ispath[i]) {
	if (tmp.find("..") != std::string::npos) {
	  std::cout << "fatal error: bad environment variable "
		    << tmp << "=" << env_vals[i] << ": has a \"..\"\n";
	  doabort = true;
	}
	/* Currently there is only one environment variable that is a filepath,
	 * namely the mount point, so this will chdir the server into the mount
	 * point. If we add more filepath environment variables, we should
	 * make this less subtle. */
	if (chdir(env_vals[i]) == -1) {
	  std::cout << "fatal error: couldn't visit "
		    << tmp << "=" << env_vals[i] << std::endl;
	  doabort = true;
	}
      }
    }
    if (doabort) abort();
  }

};
