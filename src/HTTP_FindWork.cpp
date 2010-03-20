#include "HTTP_FindWork.hpp"

HTTP_FindWork::HTTP_FindWork(size_t req_prealloc, Scheduler &sch, 
			     FileCache &cache)
  : FindWork_prealloc<HTTP_Work>(req_prealloc)
{
  HTTP_Work::sch = &sch;
  HTTP_Work::cache = &cache;
  HTTP_Work::st = &st;
}

Work *HTTP_FindWork::operator()(int fd, Work::mode m)
{
  /* TODO: think about synchronization.
   * Note that if fd is found in the state map, the second parameter
   * is ignored. Is this the right thing to do? */
  Workmap::iterator it;
  if ((it = st.find(fd)) != st.end())
    return it->second;
  else {
    HTTP_Work *w = new HTTP_Work(fd, m);
    st[fd] = w;
    return w;
  }
}
