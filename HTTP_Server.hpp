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
  /* TODO: this data structure is unsynchronized. I believe this is safe since
   * two workers never have the same file descriptor, but I need to think
   * about it more. */
  std::unordered_map<int, Work *> st;
  HTTP_Work workmaker;
public:
  HTTP_Server(char const *portno, char const *ifnam, int nworkers,
	      bool ipv6, size_t cacheszMB)
    : cache(cacheszMB * (1<<20)),
      Server((ipv6) ? AF_INET6 : AF_INET, workmaker, portno, ifnam, nworkers)
  {
    workmaker.init(&q, &sch, &cache, &st);
  }
  ~HTTP_Server()
  {
    std::unordered_map<int, Work *>::iterator it;
    for (it = st.begin(); it != st.end(); ++it) {
      dynamic_cast<HTTP_Work *>(it->second)->erasemyself = false;
      delete it->second;
    }
  }
};

#endif // HTTP_SERVER_HPP
