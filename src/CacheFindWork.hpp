#ifndef CACHE_FINDWORK_HPP
#define CACHE_FINDWORK_HPP

#include "CacheWork.hpp"
#include "FindWork_prealloc.hpp"

class CacheFindWork : public FindWork_prealloc<CacheWork>
{
  CacheFindWork(CacheFindWork const &);
  CacheFindWork &operator=(CacheFindWork const &);
public:
  CacheFindWork(size_t prealloc_MB, Scheduler &sch, FileCache &cache);
  Work *operator()(int fd, Work::mode m);
};

CacheFindWork::CacheFindWork(size_t prealloc_MB, Scheduler &sch, FileCache &cache)
  : FindWork_prealloc<CacheWork>(prealloc_MB * (1<<20))
{
  CacheWork::sch = &sch;
  CacheWork::cache = &cache;
}

Work *CacheFindWork::operator()(int fd, Work::mode m)
{
  Workmap::iterator it;
  if ((it = st.find(fd)) != st.end())
    return it->second;
  else {
    CacheWork *w = new CacheWork(fd, m);
    st[fd] = w;
    return w;
  }
}

#endif // CACHE_FINDWORK_HPP
