#include "CacheFindWork.hpp"

CacheFindWork::CacheFindWork(Scheduler &sch)
{
  CacheWork::sch = &sch;
  CacheWork::st = &st;
}

void CacheFindWork::setcache(FileCache &cache)
{
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
