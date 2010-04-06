#ifndef WORKER_HPP
#define WORKER_HPP

#include <list>

#include "ConcurrentQueue.hpp"
#include "Factory.hpp"
#include "Work.hpp"

class Worker
{
  /* Workers have no state.
   * This is on purpose: we want all state to reside either in the object
   * being worked on, so that a different worker can pick up where we
   * leave off, or be shared among all workers. Are there times when
   * we would genuinely need per-worker state? */ 
public:
  Worker() {}
  static ConcurrentQueue<Work *> *q; // where to go to get work
  void work();
private:
  Worker &operator=(Worker const&);
  Worker(Worker const&);
};

template<> class Factory<Worker>
{
public:
  Factory(ConcurrentQueue<Work *> *q)
  {
    Worker::q = q;
  }
  Worker *operator()()
  {
    return new Worker();
  }
};

#endif // WORKER_HPP
