#ifndef SERVER_HPP
#define SERVER_HPP

#include <unordered_map>

#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Thread.hpp"
#include "Worker.hpp"

class Server
{
private:
  // No copying, no assigning.
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
  // Uh oh. Needs to be synchronized.
  std::unordered_map<int, Work *> state; // Persistent state for work.

public:
  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(int domain, mkWork &makework, char const *bindto,
	 char const *ifnam, int nworkers, int listenq=10);
  void serve();
};

#endif // SERVER_HPP
