#include "HTTP_Server.h"

using namespace HTTP_env;

int main(int argc, char **argv)
{
    collectenvs(argv[0]);
    try {
      HTTP_Server(env_vals[port], env_vals[ifnam]).serve();
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
