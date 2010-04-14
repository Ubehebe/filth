#ifndef WORKER_HPP
#define WORKER_HPP

#include "ConcurrentQueue.hpp"
#include "Work.hpp"

class Worker
{
public:
  static ConcurrentQueue<Work *> *jobq; // where to go to get work
  Worker() {}
  virtual ~Worker() {}
  void work();
private:
  Worker &operator=(Worker const&);
  Worker(Worker const&);
};

#endif // WORKER_HPP
