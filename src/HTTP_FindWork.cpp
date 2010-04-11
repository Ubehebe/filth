#include "HTTP_FindWork.hpp"

HTTP_FindWork::HTTP_FindWork(size_t req_prealloc, Scheduler *sch)
  : FindWork_prealloc<HTTP_Server_Work>(req_prealloc)
{
  HTTP_Server_Work::setsch(sch);
  HTTP_Server_Work::st = &st;
}

void HTTP_FindWork::setcache(HTTP_Cache *cache)
{
  HTTP_Server_Work::cache = cache;
}

Work *HTTP_FindWork::operator()(int fd, Work::mode m)
{
  Workmap::iterator it;
  if ((it = st.find(fd)) != st.end())
    return it->second;
  else {
    HTTP_Server_Work *w = new HTTP_Server_Work(fd, m);
    st[fd] = w;
    return w;
  }
}
