#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "HTTP_env.h"
#include "HTTP_Work.h"
#include "Server_new.h"
#include "ServerErrs.h"

class HTTP_Server : public Server
{
  std::map<std::string, std::pair<time_t, std::string *> > cache;
  HTTP_mkWork makework;
public:
  HTTP_Server(char const *portno, char const *ifnam)
    : Server(AF_INET, makework, portno, ifnam)
  {
    makework.init(&q, &sch);
  }
};

#endif // HTTP_SERVER_H
