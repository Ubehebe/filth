#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include "Work.hpp"
#include "Workmap.hpp"

class FindWork
{
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
protected:
  Workmap st;
public:
  FindWork() {}
  /* The basic idea is to look up fd in the workmap. If it's there, return
   * the associated piece of work; if it's not there, make a new piece
   * of work, insert and return. */
  virtual Work *operator()(int fd, Work::mode m) = 0;
  virtual ~FindWork() {}
};

#endif // FINDWORK_HPP
