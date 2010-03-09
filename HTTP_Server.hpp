#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <iostream>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "HTTP_cmdline.hpp"
#include "HTTP_Work.hpp"
#include "Server.hpp"
#include "ServerErrs.hpp"

class HTTP_Server : public Server
{
  std::unordered_map<std::string, std::pair<time_t, std::string *> > cache;
  HTTP_mkWork makework;
public:
  HTTP_Server(int domain, char const *portno, char const *ifnam, int nworkers)
    : Server(domain, makework, portno, ifnam, nworkers)
  {
    makework.init(&q, &sch);
  }
};

#endif // HTTP_SERVER_HPP
