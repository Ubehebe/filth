#ifndef WORK_HPP
#define WORK_HPP

class Work
{
public:
  enum mode { read, write } m;
  int fd;
  virtual void operator()() = 0;
  virtual ~Work() {}
  Work(int fd, mode m) : fd(fd), m(m) {}
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
