#include <stdlib.h>
#include <string.h>
#include "HTTP_Server.h"

using namespace HTTP_env;

int main(int argc, char **argv)
{
  parsecmdline(argc, argv);
  int domain = (strncmp(flag_vals[ipv], "6", 1)==0) ? AF_INET6 : AF_INET;
    try {
      HTTP_Server(domain,
		  flag_vals[port],
		  flag_vals[ifnam],
		  atoi(flag_vals[nworkers])).serve();
    }
    catch (ResourceErr e) {
      errno = e.err;
      perror(e.msg);
      exit(1);
    }
    catch (SocketErr e) {
      errno = e.err;
      perror(e.msg);
      exit(1);
    }
}
