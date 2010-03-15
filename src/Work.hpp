#ifndef WORK_HPP
#define WORK_HPP

#include "Callback.hpp"

// Work can't be instantiated; its operator() from Callback is pure virtual.
class Work : public Callback
{
  // No copying, no assigning.
  Work(Work const&);
  Work &operator=(Work const&);

public:
  enum mode { read, write } m;
  int fd;
  bool deleteme;
  Work(int fd, mode m) : fd(fd), m(m), deleteme(false) {}
};

#endif // WORK_HPP
