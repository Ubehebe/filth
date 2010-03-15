#ifndef WORK_HPP
#define WORK_HPP

/* A minimal idea of what a "unit of work" should be in a client/server
 * system. A unit of work is associated with a file descriptor and has
 * a very general state, "read" or "write", as well as a flag telling the
 * worker if it can delete the work object when it's done processing it.
 * When a worker gets a unit of work, operator() tells it everything
 * about what to do, and arranges for any subsequent work that needs
 * to be done. For example, in a web server, a newly accepted connection
 * could be represented by a work object in read mode; this could tell the
 * worker to read from the socket until it is able to parse the HTTP request.
 * Once the response is ready, the same object can be put into the job queue
 * in write mode, telling the worker to write to the socket.
 *
 * Note that the interpretation of "read" and "write" is totally up to the
 * derived class, as is the implementation of any buffering that needs
 * to be done.
 *
 * getwork may be as simple as a call to the constructor of the derived class.
 * (If I knew more about inheritance, maybe I could get this automatically.)
 * But it has more interesting uses. For example, getwork(fd, m) might first
 * check if there is already a work object associated with fd, and if so return
 * that. */
class Work
{
  // No copying, no assigning.
  Work(Work const&);
  Work &operator=(Work const&);

public:
  enum mode { read, write } m;
  int fd;
  bool deleteme;
  Work(int fd, mode m) : fd(fd), m(m), deleteme(false) {}
  virtual Work *getwork(int fd, Work::mode m) = 0;
  virtual void operator()() = 0;
  virtual ~Work() {}
};

#endif // WORK_HPP
