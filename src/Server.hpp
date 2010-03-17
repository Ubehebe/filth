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
private:
  Server &operator=(Server const&);
  Server(Server const&);

  // Internal setup functions for supported domains.
  void setup_AF_INET(char const *bindto, char const *ifnam);
  void setup_AF_INET6(char const *bindto, char const *ifnam);
  void setup_AF_LOCAL(char const *bindto);

  int domain, listenfd, listenq, nworkers;

protected:
  LockedQueue<Work *> q; // Jobs waiting to be worked on.
  Scheduler sch;

public:
  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(int domain, FindWork &fwork, char const *bindto,
	 char const *ifnam, int nworkers, int listenq);
  void serve();
};

#endif // SERVER_HPP
