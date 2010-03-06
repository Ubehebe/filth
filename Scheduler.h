#ifndef POLLER_H
#define POLLER_H

#include <map>
#include <pthread.h>

#include "LockedQueue.h"

class Scheduler
{
  // No copying.
  Scheduler(Scheduler const &);
  Scheduler &operator=(Scheduler const &);

  bool dowork, working;
  int listenfd, pollfd, maxevents, to_ms;
  pthread_t th;
  void poll();
  static void *poll_wrapper(void *obj);

  // References to other stuff
  LockedQueue<std::pair<int, char> > &q;
  
public:
  Scheduler(LockedQueue<std::pair<int, char> > &q,
	    int pollsz=100, int maxevents=100, int to_ms=10000);
  void start(int listenfd);
  void reschedule(int fd, char mode);
  void schedule(int fd, char mode);
  void shutdown();
};

#endif // POLLER_H
