#ifndef WORKER_HPP
#define WORKER_HPP

#include <list>

#include "ConcurrentQueue.hpp"
#include "Work.hpp"

class Worker
{
  // No copying, no assigning.
  Worker &operator=(Worker const&);
  Worker(Worker const&);

public:
  /* The intent is to set the static q before calling any constructors. */
  Worker() {}
  static ConcurrentQueue<Work *> *q; // where to go to get work
  void work();
};

#endif // WORKER_HPP
