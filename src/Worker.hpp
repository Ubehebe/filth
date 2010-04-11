#ifndef WORKER_HPP
#define WORKER_HPP

#include <list>

#include "ConcurrentQueue.hpp"
#include "Factory.hpp"
#include "logging.h"
#include "Work.hpp"

class Worker
{
public:
  Worker() {}
  virtual ~Worker() {}
  static ConcurrentQueue<Work *> *q; // where to go to get work
  void work();
  virtual void imbue_state(Work *w) { _LOG_DEBUG("base!\n"); }
private:
  Worker &operator=(Worker const&);
  Worker(Worker const&);
};

template<> class Factory<Worker>
{
public:
  void setq(ConcurrentQueue<Work *> *q)
  {
    Worker::q = q;
  }
  virtual Worker *operator()()
  {
    return new Worker();
  }
};

#endif // WORKER_HPP
