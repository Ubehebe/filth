#include <stdlib.h>
#include <string.h>
#include "HTTP_Server.hpp"

using namespace HTTP_cmdline;

int main(int argc, char **argv)
{
  parsecmdline(argc, argv);
  try {
    HTTP_Server(
		HTTP_cmdline::svals[HTTP_cmdline::port],
		HTTP_cmdline::svals[HTTP_cmdline::ifnam],
		HTTP_cmdline::ivals[HTTP_cmdline::nworkers],
		HTTP_cmdline::bvals[HTTP_cmdline::ipv6],
		HTTP_cmdline::ivals[HTTP_cmdline::cachesz]).serve();
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
