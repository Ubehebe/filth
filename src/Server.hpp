#ifndef SERVER_HPP
#define SERVER_HPP

#include <unordered_map>

#include "FindWork.hpp"
#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Thread.hpp"
#include "Worker.hpp"

class Server
{
  Server &operator=(Server const&);
  Server(Server const&);

  // Internal setup functions for supported domains.
  void setup_AF_INET(char const *ifnam);
  void setup_AF_INET6(char const *ifnam);
  void setup_AF_LOCAL();

  int domain, listenfd, listenq, nworkers;
  char const *bindto;
  char *sockdir;

  LockedQueue<Work *> q; // Jobs waiting to be worked on.

public:
  Scheduler sch;
  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(int domain, FindWork &fwork, char const *mount, char const *bindto,
	 int nworkers, int listenq, char const *ifname=NULL);
  ~Server();
  void serve();
};

#endif // SERVER_HPP
