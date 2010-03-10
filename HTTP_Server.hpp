#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "FileCache.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_Work.hpp"
#include "Server.hpp"
#include "ServerErrs.hpp"

class HTTP_Server : public Server
{
  FileCache cache;
  HTTP_mkWork makework;
public:
  HTTP_Server(char const *portno, char const *ifnam, int nworkers,
	      bool ipv6, size_t cacheszMB)
    : cache(cacheszMB * (1<<20)),
      Server((ipv6) ? AF_INET6 : AF_INET, makework, portno, ifnam, nworkers)
  {
    makework.init(&q, &sch, &cache);
  }
};

#endif // HTTP_SERVER_HPP
