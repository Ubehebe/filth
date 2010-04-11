#ifndef HTTP_FINDWORK_HPP
#define HTTP_FINDWORK_HPP

#include <stdint.h>
#include <sys/types.h>

#include "HTTP_Cache.hpp"
#include "HTTP_Server_Work.hpp"
#include "FindWork_prealloc.hpp"
#include "logging.h"
#include "Scheduler.hpp"
#include "Workmap.hpp"

class HTTP_FindWork : public FindWork_prealloc<HTTP_Server_Work>
{
public:
  HTTP_FindWork(size_t req_prealloc, Scheduler *sch);
  ~HTTP_FindWork() {}
  void setcache(HTTP_Cache *cache);
  Work *operator()(int fd, Work::mode m);
private:
  HTTP_FindWork(HTTP_FindWork const&);
  HTTP_FindWork &operator=(HTTP_FindWork const&);
};

#endif // HTTP_FINDWORK_HPP
