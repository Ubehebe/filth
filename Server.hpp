#ifndef SERVER_HPP
#define SERVER_HPP

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
  LockedQueue<Work *> q;
  Scheduler sch;

public:
  Server(int domain, mkWork &makework, char const *bindto,
	 char const *ifnam, int nworkers, int listenq=10);
  void serve();
};

#endif // SERVER_HPP
