#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include "Work.hpp"

class FindWork
{
  // No copying, no assigning.
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
public:
  FindWork() {}
  virtual Work *operator()(int fd, Work::mode m) = 0;
  virtual ~FindWork() {}
};

#endif // FINDWORK_HPP
