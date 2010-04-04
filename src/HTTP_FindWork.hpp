#ifndef HTTP_FINDWORK_HPP
#define HTTP_FINDWORK_HPP

#include <stdint.h>
#include <sys/types.h>

#include "HTTP_Cache.hpp"
#include "FindWork_prealloc.hpp"
#include "Workmap.hpp"
#include "HTTP_Work.hpp"
#include "Scheduler.hpp"

class HTTP_FindWork : public FindWork_prealloc<HTTP_Work>
{
  HTTP_FindWork(HTTP_FindWork const&);
  HTTP_FindWork &operator=(HTTP_FindWork const&);
public:
  HTTP_FindWork(size_t req_prealloc, Scheduler &sch);
  void setcache(HTTP_Cache &cache);
  
  Work *operator()(int fd, Work::mode m);
};

#endif // HTTP_FINDWORK_HPP
