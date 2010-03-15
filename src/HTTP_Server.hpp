#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "HTTP_cmdline.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_Statemap.hpp"
#include "HTTP_FindWork.hpp"
#include "inotifyFileCache.hpp"
#include "logging.h"
#include "Server.hpp"
#include "ServerErrs.hpp"

class HTTP_Server : public Server
{
  // No copying, no assigning.
  HTTP_Server(HTTP_Server const&);
  HTTP_Server &operator=(HTTP_Server const&);

  inotifyFileCache cache;
  HTTP_Statemap st;
  HTTP_FindWork fwork;
public:
  HTTP_Server(char const *portno,
	      char const *ifnam,
	      char const *mount,
	      int nworkers,
	      bool ipv6,
	      size_t cacheszMB);
};

HTTP_Server::HTTP_Server(char const *portno,
			 char const *ifnam,
			 char const *mount,
			 int nworkers,
			 bool ipv6,
			 size_t cacheszMB)
  : Server((ipv6) ? AF_INET6 : AF_INET, fwork, portno, ifnam, nworkers),
    cache(cacheszMB * (1<<20), fwork, sch),
    fwork(q, sch, cache, st)
{
  if (chdir(mount)==-1) {
    _LOG_FATAL("chdir: %m");
    exit(1);
  }
}

#endif // HTTP_SERVER_HPP
