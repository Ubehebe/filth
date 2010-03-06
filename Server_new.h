#ifndef SERVER_H
#define SERVER_H

#include "LockedQueue.h"
#include "Scheduler_new.h"
#include "Thread.h"
#include "Worker_new.h"

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

  // Because they go to the scheduler.
  void block_all_signals();

  int domain, listenfd, listenq, nworkers;

protected:
  LockedQueue<Work *> q;
  Scheduler sch;

public:
  Server(int domain, mkWork &makework, char const *bindto,
	 char const *ifnam, int nworkers, int listenq=10);
  void serve();
};

#endif // SERVER_H
