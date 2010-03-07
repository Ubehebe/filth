#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <map>
#include <sys/types.h>
#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

// Forward declaration for befriending HTTP_Work.
class HTTP_mkWork;

class HTTP_Work : public Work
{
  friend class HTTP_mkWork;
  // No copying, no assigning.
  HTTP_Work(HTTP_Work const &);
  HTTP_Work &operator=(HTTP_Work const &);
  
  static const size_t bufsz = 1<<10;
  static LockedQueue<Work *> *q;
  static Scheduler *sch;
 
public:
  void operator()();
  HTTP_Work(int fd, Work::mode m) : Work(fd, m) {}
};

class HTTP_mkWork : public mkWork
{
public:
  Work *operator()(int fd, Work::mode m);
  /* Rationale: we make a new (meaningless) HTTP_mkWork object
   * as a temporary in the HTTP_Server constructor. It is passed around
   * in the Server constructor, since the scheduler and the workers need
   * to know how to make new work objects. An HTTP_Work object is a
   * particular kind of Work object: it needs to know about the work
   * queue and the scheduler, since it might need to initiate new work.
   * But the work queue and the scheduler don't exist until the server
   * constructor returns. Thus we should call init in the body of the
   * HTTP_Server constructor to set them. Note that this means that we
   * can't actually start the server from inside the server constructor; we
   * provide a serve() function instead. */
  void init(LockedQueue<Work *> *_q, Scheduler *sch);
};

#endif // HTTP_WORK_HPP
