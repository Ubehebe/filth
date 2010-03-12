#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "FileCache.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_Statemap.hpp"
#include "HTTP_Work.hpp"
#include "logging.h"
#include "Server.hpp"
#include "ServerErrs.hpp"

class HTTP_Server : public Server
{
  FileCache cache;
  HTTP_Statemap st;
  HTTP_Work workmaker;
public:
  HTTP_Server(char const *portno,
	      char const *ifnam,
	      char const *mount,
	      int nworkers,
	      bool ipv6,
	      size_t cacheszMB);
  ~HTTP_Server();
};

HTTP_Server::HTTP_Server(char const *portno, char const *ifnam,
			 char const *mount, int nworkers, bool ipv6, size_t cacheszMB)
  : cache(cacheszMB * (1<<20)),
    Server((ipv6) ? AF_INET6 : AF_INET, workmaker, portno, ifnam, nworkers)
{
  if (chdir(mount)==-1) {
    _LOG_ERR("%m");
    exit(1);
  }
  workmaker.init(&q, &sch, &cache, &st);
}

HTTP_Server::~HTTP_Server()
{
  std::list<Work *> todel;
  for (HTTP_Statemap::iterator it = st.begin(); it != st.end(); ++it)
    todel.push_back(it->second);
  for (std::list<Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
    delete *it;
}


#endif // HTTP_SERVER_HPP
