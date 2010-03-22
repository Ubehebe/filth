#ifndef WORKER_HPP
#define WORKER_HPP

#include <list>

#include "LockedQueue.hpp"
#include "Work.hpp"

class Worker
{
  // No copying, no assigning.
  Worker &operator=(Worker const&);
  Worker(Worker const&);

public:
  /* The intent is to set the static q before calling any constructors. */
  Worker() {}
  static LockedQueue<Work *> *q; // where to go to get work
  void work();
};

#endif // WORKER_HPP
