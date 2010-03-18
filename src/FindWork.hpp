#ifndef FINDWORK_HPP
#define FINDWORK_HPP

#include <list>

#include "Work.hpp"
#include "Workmap.hpp"

class FindWork
{
  FindWork(FindWork const&);
  FindWork &operator=(FindWork const&);
protected:
  Workmap st;
public:
  void clear_Workmap()
  {
    /* The reason for this strange pattern is that the derived Work destructor
     * should remove itself from the statemap, which would invalidate our
     * iterator. */
    std::list<Work *> todel;
    for (Workmap::iterator it = st.begin(); it != st.end(); ++it)
      todel.push_back(it->second);
    for (std::list<Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
      delete *it;
    st.clear();
  }
  FindWork() {}
  /* The basic idea is to look up fd in the workmap. If it's there, return
   * the associated piece of work; if it's not there, make a new piece
   * of work, insert and return. */
  virtual Work *operator()(int fd, Work::mode m) = 0;
  virtual ~FindWork() { clear_Workmap(); }
};

#endif // FINDWORK_HPP
