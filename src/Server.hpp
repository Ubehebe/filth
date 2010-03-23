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
  int sigdeadlock_internal, sigdeadlock_external;

protected:
  Scheduler sch;

public:

  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(
	 int domain,
	 FindWork &fwork,
	 char const *mount,
	 char const *bindto,
	 int nworkers,
	 int listenq,
	 int sigdeadlock_internal,
	 int sigdeadlock_external,
	 char const *ifname=NULL);
  ~Server();
  void serve();
};

#endif // SERVER_HPP
