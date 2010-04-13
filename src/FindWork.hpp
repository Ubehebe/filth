#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include "Work.hpp"

/* This class is like a Factory for pieces of work. But instead of always
 * returning a new piece of work, we could keep an internal map, returning
 * a pointer to a suitable old piece of work if it already exists. */
class FindWork
{
public:
  FindWork() {}
  virtual Work *operator()(int fd, Work::mode m) = 0;
  virtual ~FindWork() {}
private:
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
};

#endif // FINDWORK_HPP
