#ifndef UNIX_DOMAIN_SERVER_H
#define UNIX_DOMAIN_SERVER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "CGI_Parsing.h"
#include "Server.h"


class Unix_Domain_Server : public Server
{
  CGI_Parsing cpars;
public:
  Unix_Domain_Server(char const *path, void (*handle_SIGINT)(int),
		     void (*at_exit)(void))
    : Server(AF_LOCAL, path, handle_SIGINT, at_exit, cpars) {}
};

#endif // UNIX_DOMAIN_SERVER_H
