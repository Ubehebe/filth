#include "FindWork.hpp"

template<class _Work> void FindWork<_Work>::clear()
{
  /* The reason for this strange pattern is that the derived Work destructor
   * should remove itself from wmap, which would invalidate our
   * iterator. */
  std::list<_Work *> todel;
  wmaplock.wrlock();
  for (workmap::iterator it = wmap.begin(); it != wmap.end(); ++it)
    todel.push_back(it->second);
  for (std::list<_Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
    delete *it;
  wmap.clear();
  wmaplock.unlock();
}

template<class _Work> _Work *FindWork<_Work>::operator()(int fd, Work::mode m)
{
  workmap::iterator it;
  wmaplock.rdlock();
  if ((it = wmap.find(fd)) != wmap.end()) {
    wmaplock.unlock();
    return it->second;
  }
  else {
    _Work *w = new _Work(fd,m);
    st[fd] = w;
    wmaplock.unlock();
    return w;
  }
}
