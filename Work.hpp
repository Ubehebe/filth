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
 * to be done. */
class Work
{
public:
  enum mode { read, write } m;
  int fd;
  bool deleteme;
  virtual void operator()() = 0;
  virtual ~Work() {}
  Work(int fd, mode m) : fd(fd), m(m), deleteme(false) {}
};

/* Why not just have a pure virtual function makework in the work class?
 * Well, for some reason you can't have virtual static functions. A nonstatic
 * makework function sounds like a bad idea, since (at least one) work object
 * is allocated for each incoming request. */
class mkWork
{
public:
  virtual Work *operator()(int fd, Work::mode m) = 0;
};

#endif // WORK_HPP
