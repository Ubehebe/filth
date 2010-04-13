#ifndef FINDWORK_PREALLOC_HPP
#define FINDWORK_PREALLOC_HPP

#include <list>

#include "FindWork.hpp"
#include "Locks.hpp"
#include "logging.h"

/* The _Work class needs to be descended from Preallocated or you will get
 * template instantiation errors. */
template<class _Work> class FindWork_prealloc : public FindWork
{
public:
  FindWork_prealloc(size_t prealloc_bytes);
  ~FindWork_prealloc();
  Work *operator()(int fd, Work::mode m);
  typedef std::unordered_map<int, _Work *> workmap;
private:
  workmap wmap;
  RWLock wmaplock;
  void clear();
  FindWork_prealloc(FindWork_prealloc const &);
  FindWork_prealloc &operator=(FindWork_prealloc const &);
};

template<class _Work>
FindWork_prealloc<_Work>::FindWork_prealloc(size_t prealloc_bytes)
{
  size_t prealloc_chunks = prealloc_bytes / sizeof(_Work);
  _Work::prealloc_init(prealloc_chunks);
  _Work::wmap = &wmap;
}

template<class _Work>
FindWork_prealloc<_Work>::~FindWork_prealloc()
{
  /* Send everything still in the work map back to the free store.
   * Note that the FindWork destructor will also call clear, but this isn't a
   * problem because by that time the work map is empty. */
  clear();
  _Work::prealloc_deinit();
}

template<class _Work>
void FindWork_prealloc<_Work>::clear()
{
  /* The reason for this strange pattern is that the derived Work destructor
   * should remove itself from wmap, which would invalidate our
   * iterator. */
  std::list<_Work *> todel;
  wmaplock.wrlock();
  for (typename workmap::iterator it = wmap.begin(); it != wmap.end(); ++it)
    todel.push_back(it->second);
  for (typename std::list<_Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
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
