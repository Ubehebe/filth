#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "HTTP_cmdline.h"
#include "HTTP_Work.h"
#include "Server_new.h"
#include "ServerErrs.h"

class HTTP_Server : public Server
{
  std::map<std::string, std::pair<time_t, std::string *> > cache;
  HTTP_mkWork makework;
public:
  HTTP_Server(int domain, char const *portno, char const *ifnam, int nworkers)
    : Server(domain, makework, portno, ifnam, nworkers)
  {
    makework.init(&q, &sch);
  }
};

#endif // HTTP_SERVER_H
