#ifndef HTTP_SERVER_2616_HPP
#define HTTP_SERVER_2616_HPP

#include <list>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "Cache.hpp"
#include "Factory.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_FindWork.hpp"
#include "HTTP_Worker.hpp"
#include "logging.h"
#include "Server.hpp"
#include "ServerErrs.hpp"
#include "Workmap.hpp"

class HTTP_Server : public Server
{
public:
  HTTP_Server(char const *portno,
	      char const *ifnam,
	      char const *mount,
	      int nworkers,
	      bool ipv6,
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq,
	      int sigflush,
	      int sigdl_int,
	      int sigdl_ext,
	      int tcp_keepalive_intvl,
	      int tcp_keepalive_probes,
	      int tcp_keepalive_time);
  void onstartup();
  void onshutdown();
  ~HTTP_Server();
  #ifdef _COLLECT_STATS
  uint32_t flushes;
#endif // _COLLECT_STATS

private:
  HTTP_2616_Server(HTTP_2616_Server const&);
  HTTP_2616_Server &operator=(HTTP_2616_Server const&);

  HTTP_FindWork *fwork;
  HTTP_Cache *cache;

  static HTTP_Server *theserver; // For non-signalfd-based signal handling
  static void flush(int ignore=-1);

  size_t req_prealloc_MB, cacheszMB;
  int sigflush;
};

#endif // HTTP_SERVER_2616_HPP
