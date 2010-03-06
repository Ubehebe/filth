#ifndef WORKER_H
#define WORKER_H

#include <map>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <sys/types.h>

#include "LockedQueue.h"
#include "Parsing.h"
#include "Scheduler.h"

class Worker
{
  // No copying.
  Worker &operator=(Worker const&);
  Worker(Worker const&);

  size_t bufsz;
  bool dowork;
  pthread_t th;
  time_t to_sec;

  /* Pointers to stuff that is elsewhere, static because they should be the
   * same for all workers. */
  static LockedQueue<std::pair<int, char> > *q;
  static Scheduler *sch;
  static std::map<int, std::pair<std::stringstream *, Parsing *> > *state;
  static Parsing *pars;

  static void *work_wrapper(void *worker);
  void operator()();
public:
  Worker(LockedQueue<std::pair<int, char> > *q, Scheduler *sch,
	 std::map<int, std::pair<std::stringstream *, Parsing *> > *state,
	 Parsing *pars, time_t to_sec, size_t bufsz = 1<<10);
  void shutdown();
};

#endif // WORKER_H
