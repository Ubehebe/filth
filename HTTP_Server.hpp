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
#include "HTTP_constants.hpp"
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

HTTP_Server::HTTP_Server(char const *portno,
			 char const *ifnam,
			 char const *mount,
			 int nworkers,
			 bool ipv6,
			 size_t cacheszMB)
  : Server((ipv6) ? AF_INET6 : AF_INET, workmaker, portno, ifnam, nworkers),
    cache(cacheszMB * (1<<20), &sch, &workmaker)
{
  HTTP_constants::cpp_token_tinker(HTTP_constants::status_strs,
				   HTTP_constants::num_status,
				   '_', ' ');
  //  HTTP_constants::cpp_token_tinker(HTTP_constants::header_strs,
  //				   HTTP_constants::num_header,
  //				   '_', '-');
  workmaker.static_init(&q, &sch, &cache, &st);
  sch.register_special_fd(cache.inotifyfd, cache.inotify_cb, Work::read);
  if (chdir(mount)==-1) {
    _LOG_CRIT("%m");
    exit(1);
  }
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
