#include <sys/types.h>
#include <sys/socket.h>

#include "logging.h"
#include "HTTP_2616_Server_Work.hpp"
#include "HTTP_2616_Worker.hpp"
#include "HTTP_CacheEntry.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_parsing.hpp"
#include "CachingServer.hpp"

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
  HTTP_Server_Work::setmaxes(c.ivals[HTTP_cmdline::max_req_uri],
			     c.ivals[HTTP_cmdline::max_req_body] * (1<<20));
  HTTP_parsing::setmaxes(c.ivals[HTTP_cmdline::max_req_hdr]);
  try {
    CachingServer<HTTP_2616_Server_Work, HTTP_2616_Worker, HTTP_CacheEntry *>
      (c.bvals[HTTP_cmdline::ipv6] ? AF_INET6 : AF_INET,
       c.svals[HTTP_cmdline::mount],
       c.svals[HTTP_cmdline::port],
       c.ivals[HTTP_cmdline::cachesz],
       c.sigconv(c.svals[HTTP_cmdline::sigflush]),
       c.ivals[HTTP_cmdline::nworkers],
       c.ivals[HTTP_cmdline::listenq],
       c.ivals[HTTP_cmdline::req_prealloc_sz],
       c.svals[HTTP_cmdline::ifnam],
       NULL,
       c.sigconv(c.svals[HTTP_cmdline::sigdl_int]),
       c.sigconv(c.svals[HTTP_cmdline::sigdl_ext]),
       c.ivals[HTTP_cmdline::tcp_keepalive_intvl],
       c.ivals[HTTP_cmdline::tcp_keepalive_probes],
       c.ivals[HTTP_cmdline::tcp_keepalive_time],
       c.ivals[HTTP_cmdline::untrusted_uid],
       c.ivals[HTTP_cmdline::untrusted_gid]
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
