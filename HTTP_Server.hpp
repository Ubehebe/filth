#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "FileCache.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_Statemap.hpp"
#include "HTTP_Work.hpp"
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
    perror("chdir");
    exit(1);
  }
  workmaker.init(&q, &sch, &cache, &st);
}

HTTP_Server::~HTTP_Server()
{
  HTTP_Statemap::iterator it;
  for (it = st.begin(); it != st.end(); ++it) {
    dynamic_cast<HTTP_Work *>(it->second)->erasemyself = false;
    delete it->second;
  }
}


#endif // HTTP_SERVER_HPP
