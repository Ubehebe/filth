#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "HTTP_cmdline.hpp"
#include "HTTP_Server.hpp"

using HTTP_cmdline::c;

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
		c.svals[HTTP_cmdline::port],
		c.svals[HTTP_cmdline::ifnam],
		c.svals[HTTP_cmdline::mount],
		c.ivals[HTTP_cmdline::nworkers],
		c.bvals[HTTP_cmdline::ipv6],
		c.ivals[HTTP_cmdline::cachesz],
		c.ivals[HTTP_cmdline::req_prealloc_sz],
		c.ivals[HTTP_cmdline::listenq],
		c.sigconv(c.svals[HTTP_cmdline::sigflush]),
		c.sigconv(c.svals[HTTP_cmdline::sigdl_int]),
		c.sigconv(c.svals[HTTP_cmdline::sigdl_ext]),
		c.ivals[HTTP_cmdline::tcp_keepalive_intvl],
		c.ivals[HTTP_cmdline::tcp_keepalive_probes],
		c.ivals[HTTP_cmdline::tcp_keepalive_time]
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
