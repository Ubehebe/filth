#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <sstream>
#include <vector>
#include "LockedQueue.h"
#include "Scheduler.h"
#include "Work.h"
#include "Worker.h"

class Server
{
  // No copying, no assigning.
  Server &operator=(Server const&);
  Server(Server const&);

protected:
  Scheduler sch;
  std::vector<Worker *> workers;
  LockedQueue<Work *> q;
  std::map<int, std::pair<std::stringstream *, Parsing *> > state;

  int domain, listenfd, listenq, nworkers;
  time_t to_sec;
  Parsing &pars;
  
  // Helper functions.
  static void deleteWorker(Worker *w);
  static void deleteState(std::pair<int,
			  std::pair<std::stringstream *, Parsing *> > p);

public:
  Server(int domain, char const *bindto, void (*handle_SIGINT)(int), void (*at_exit)(),
	 Parsing &pars, int nworkers=10, time_t to_sec=10, int listenq=10);
  ~Server();
  // "Run" the server; should block until the scheduler shuts down.
  void operator()() { sch.start(listenfd); }
};

#endif // SERVER_H
