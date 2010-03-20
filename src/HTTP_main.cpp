#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "HTTP_cmdline.hpp"
#include "HTTP_Server.hpp"

void logatexit()
{
  _LOG_INFO("exiting");
}

int main(int argc, char **argv)
{
  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  SYSLOG_OPTS, LOG_USER);
  atexit(logatexit);
  _LOG_INFO("starting");
  HTTP_cmdline::cmdlinesetup(argc, argv);
  try {
    HTTP_Server(
		HTTP_cmdline::c.svals[HTTP_cmdline::port],
		HTTP_cmdline::c.svals[HTTP_cmdline::ifnam],
		HTTP_cmdline::c.svals[HTTP_cmdline::mount],
		HTTP_cmdline::c.ivals[HTTP_cmdline::nworkers],
		HTTP_cmdline::c.bvals[HTTP_cmdline::ipv6],
		HTTP_cmdline::c.ivals[HTTP_cmdline::cachesz],
		HTTP_cmdline::c.ivals[HTTP_cmdline::req_prealloc_sz],
		HTTP_cmdline::c.ivals[HTTP_cmdline::listenq],
		HTTP_cmdline::c.sigconv(HTTP_cmdline::c.svals[HTTP_cmdline::sigflush])
		).serve();
  }
  catch (ResourceErr e) {
    _LOG_FATAL("uncaught ResourceErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
  catch (SocketErr e) {
    _LOG_FATAL("uncaught SocketErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
}
