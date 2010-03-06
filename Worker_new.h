#ifndef WORKER_H
#define WORKER_H

#include "LockedQueue.h"
#include "Work.h"

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

#endif // WORKER_H
