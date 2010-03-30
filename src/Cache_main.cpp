#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Cache_cmdline.hpp"
#include "CacheServer.hpp"
#include "logging.h"

void logatexit()
{
  _LOG_INFO("exiting");
}

int main(int argc, char **argv)
{

  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  SYSLOG_OPTS, LOG_USER);
  atexit(logatexit);
  Cache_cmdline::cmdlinesetup(argc, argv);
  try {
    CacheServer(
		Cache_cmdline::c.svals[Cache_cmdline::name],
		Cache_cmdline::c.svals[Cache_cmdline::mount],
		Cache_cmdline::c.ivals[Cache_cmdline::nworkers],
		Cache_cmdline::c.ivals[Cache_cmdline::sz],
		1, // ?
		Cache_cmdline::c.ivals[Cache_cmdline::listenq],
		Cache_cmdline::c.sigconv(Cache_cmdline::c.svals[Cache_cmdline::sigdl_int]),
		Cache_cmdline::c.sigconv(Cache_cmdline::c.svals[Cache_cmdline::sigdl_ext]),
		Cache_cmdline::c.sigconv(Cache_cmdline::c.svals[Cache_cmdline::sigflush])
		).serve();
  } catch (ResourceErr e) {
    _LOG_FATAL("uncaught ResourceErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  } catch (SocketErr e) {
    _LOG_FATAL("uncaught SocketErr: %s: %s", e.msg, strerror(e.err));
    exit(1);
  }
}
