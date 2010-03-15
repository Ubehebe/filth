#ifndef HTTP_FINDWORK_HPP
#define HTTP_FINDWORK_HPP

#include <sys/types.h>

#include "FileCache.hpp"
#include "FindWork.hpp"
#include "HTTP_Statemap.hpp"
#include "HTTP_Work.hpp"
#include "LockedQueue.hpp"
#include "Scheduler.hpp"

class HTTP_FindWork : public FindWork
{
  // No copying, no assigning.
  HTTP_FindWork(HTTP_FindWork const&);
  HTTP_FindWork &operator=(HTTP_FindWork const&);

  HTTP_Statemap &st;
public:
  HTTP_FindWork(LockedQueue<Work *> &q, Scheduler &sch,
		FileCache &cache, HTTP_Statemap &st);
  ~HTTP_FindWork();
  Work *operator()(int fd, Work::mode m);
};

HTTP_FindWork::HTTP_FindWork(LockedQueue<Work *> &q, Scheduler &sch,
			     FileCache &cache, HTTP_Statemap &st)
  : st(st)
{
  HTTP_Work::sch = &sch;
  HTTP_Work::cache = &cache;
  HTTP_Work::st = &st;
}

HTTP_FindWork::~HTTP_FindWork()
{
  /* The reason for this strange pattern is that the HTTP_Work destructor
   * removes itself from the statemap, which would invalidate our iterator. */
  std::list<Work *> todel;
  for (HTTP_Statemap::iterator it = st.begin(); it != st.end(); ++it)
    todel.push_back(it->second);
  for (std::list<Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
  delete *it;
}


Work *HTTP_FindWork::operator()(int fd, Work::mode m)
{
  /* TODO: think about synchronization.
   * Note that if fd is found in the state map, the second parameter
   * is ignored. Is this the right thing to do? */
  HTTP_Statemap::iterator it;
  if ((it = st.find(fd)) != st.end())
    return it->second;
  else {
    HTTP_Work *w = new HTTP_Work(fd, m);
    st[fd] = w;
    return w;
  }
}
  

#endif // HTTP_FINDWORK_HPP
