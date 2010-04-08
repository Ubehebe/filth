#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include <list>

#include "Work.hpp"
#include "Workmap.hpp"

class FindWork
{
public:
  FindWork() {}
  void clear_Workmap();
  /* The basic idea is to look up fd in the workmap. If it's there, return
   * the associated piece of work; if it's not there, make a new piece
   * of work, insert and return. */
  virtual Work *operator()(int fd, Work::mode m) = 0;
  virtual ~FindWork() { clear_Workmap(); }
 protected:
  Workmap st;
private:
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
};

#endif // FINDWORK_HPP
