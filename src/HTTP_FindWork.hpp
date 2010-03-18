#ifndef HTTP_FINDWORK_HPP
#define HTTP_FINDWORK_HPP

#include <stdint.h>
#include <sys/types.h>

#include "FileCache.hpp"
#include "FindWork_prealloc.hpp"
#include "Workmap.hpp"
#include "HTTP_Work.hpp"
#include "Scheduler.hpp"

class HTTP_FindWork : public FindWork_prealloc<HTTP_Work>
{
  HTTP_FindWork(HTTP_FindWork const&);
  HTTP_FindWork &operator=(HTTP_FindWork const&);
public:
  HTTP_FindWork(size_t req_prealloc, Scheduler &sch, FileCache &cache);
  Work *operator()(int fd, Work::mode m);
};

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

#endif // HTTP_FINDWORK_HPP
