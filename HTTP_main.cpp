#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "HTTP_Server.hpp"

using namespace HTTP_cmdline;

void logatexit()
{
  _LOG_INFO("exiting");
}

int main(int argc, char **argv)
{
  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  0, LOG_HTTP);
  atexit(logatexit);
  _LOG_INFO("main: starting");
  parsecmdline(argc, argv);
  try {
    HTTP_Server(
		HTTP_cmdline::svals[HTTP_cmdline::port],
		HTTP_cmdline::svals[HTTP_cmdline::ifnam],
		HTTP_cmdline::svals[HTTP_cmdline::mount],
		HTTP_cmdline::ivals[HTTP_cmdline::nworkers],
		HTTP_cmdline::bvals[HTTP_cmdline::ipv6],
		HTTP_cmdline::ivals[HTTP_cmdline::cachesz]).serve();
  }
  catch (ResourceErr e) {
    _LOG_CRIT("main: uncaught ResourceErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
  catch (SocketErr e) {
    _LOG_CRIT("main: uncaught SocketErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
}
