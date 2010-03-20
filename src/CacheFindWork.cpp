#include "CacheFindWork.hpp"

CacheFindWork::CacheFindWork(size_t prealloc_bytes, Scheduler &sch, FileCache &cache)
  : FindWork_prealloc<CacheWork>(prealloc_bytes)
{
  CacheWork::sch = &sch;
  CacheWork::cache = &cache;
  CacheWork::st = &st;
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
