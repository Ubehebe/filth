#ifndef WORKER_HPP
#define WORKER_HPP

#include <list>

#include "ConcurrentQueue.hpp"
#include "Factory.hpp"
#include "Work.hpp"

class Worker
{
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
