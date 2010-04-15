#ifndef WORKER_HPP
#define WORKER_HPP

#include "ConcurrentQueue.hpp"
#include "Work.hpp"

// Fwd decls for ptrs
class Scheduler;
class FindWork;

class Worker
{
public:
  static ConcurrentQueue<Work *> *jobq; //!< where to go to get work
  static Scheduler *sch; //!< saved in case a piece of work needs it
  static FindWork *fwork; //!< saved in case a piece of work needs it
  Worker() {}
  virtual ~Worker() {}
  /** \brief Main work loop.
   * Gets a piece of work from jobq. (If the piece of work is actually a
   * null pointer, it means the worker should retire.) Performs the requested
   * work, checks to see if the work is finished, deleting it if so,
   * and loops.*/
  void work();
private:
  Worker &operator=(Worker const&);
  Worker(Worker const&);
};

#endif // WORKER_HPP
