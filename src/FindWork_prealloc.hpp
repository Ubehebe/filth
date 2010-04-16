#ifndef FINDWORK_PREALLOC_HPP
#define FINDWORK_PREALLOC_HPP

#include <list>

#include "FindWork.hpp"
#include "Locks.hpp"
#include "logging.h"

class Scheduler; // fwd declaration for ptr

/** \brief An implementation of the FindWork interface for objects deriving
 * from Preallocated. */
template<class _Work> class FindWork_prealloc : public FindWork
{
public:
  FindWork_prealloc(size_t prealloc_bytes);
  ~FindWork_prealloc();
  Work *operator()(int fd, Work::mode m);
  void unregister(int fd);
  bool register_alien(Work *w);

private:
  typedef std::unordered_map<int, Work *> workmap;
  static workmap wmap;
  RWLock wmaplock;

  void clear();
  FindWork_prealloc(FindWork_prealloc const &);
  FindWork_prealloc &operator=(FindWork_prealloc const &);
};

template<class _Work>
std::unordered_map<int, Work *> FindWork_prealloc<_Work>::wmap;

template<class _Work>
FindWork_prealloc<_Work>::FindWork_prealloc(size_t prealloc_bytes)
{
  size_t prealloc_chunks = prealloc_bytes / sizeof(_Work);
  _Work::prealloc_init(prealloc_chunks);
}

template<class _Work>
void FindWork_prealloc<_Work>::unregister(int fd)
{
  /* This is unsynchronized to avoid deadlock in clear(). 
   * That function calls delete on each item in the workmap while
   * holding the write lock, and the derived work destructor could well call
   * unregister(). Because we are striving to maintain the "one fd, one owner"
   * invariant throughout the architecture, I do not think we need to worry
   * about race conditions with two workers trying to unregister the
   * same fd. */
  wmap.erase(fd);
}

template<class _Work>
bool FindWork_prealloc<_Work>::register_alien(Work *w)
{
  if (w == NULL) return false;
  wmaplock.rdlock();
  bool okay;
  if (okay = (wmap.find(w->fd) == wmap.end()))
    wmap[w->fd] = w;
  wmaplock.unlock();
  return okay;
}

template<class _Work>
FindWork_prealloc<_Work>::~FindWork_prealloc()
{
  /* Send everything still in the work map back to the free store. */
  clear();
  _Work::prealloc_deinit();
}

template<class _Work>
void FindWork_prealloc<_Work>::clear()
{
  /* The reason for this strange pattern is that the derived Work destructor
   * should remove itself from wmap, which would invalidate our
   * iterator. */
  std::list<Work *> todel;
  wmaplock.wrlock();
  for (typename workmap::iterator it = wmap.begin(); it != wmap.end(); ++it)
    todel.push_back(it->second);
  for (typename std::list<Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
    delete *it;
  wmap.clear();
  wmaplock.unlock();
}

template<class _Work>
Work *FindWork_prealloc<_Work>::operator()(int fd, Work::mode m)
{
  typename workmap::iterator it;
  wmaplock.rdlock();
  if ((it = wmap.find(fd)) != wmap.end()) {
    wmaplock.unlock();
    return it->second; // Race condition here, but never happens in our app
  }
  else {
    _Work *w = new _Work(fd,m);
    wmap[fd] = w;
    wmaplock.unlock();
    return w;
  }
}

#endif // FINDWORK_PREALLOC_HPP
