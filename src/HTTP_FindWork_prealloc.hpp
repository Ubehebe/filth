#ifndef HTTP_FINDWORK_PREALLOC_HPP
#define HTTP_FINDWORK_PREALLOC_HPP

#include <stdint.h>
#include <sys/types.h>

#include "HTTP_Cache.hpp"
#include "HTTP_Server_Work_big.hpp"
#include "FindWork_prealloc.hpp"
#include "logging.h"
#include "Scheduler.hpp"
#include "Workmap.hpp"

class HTTP_FindWork_prealloc : public FindWork_prealloc<HTTP_Server_Work_big>
{
public:
  HTTP_FindWork_prealloc(size_t req_prealloc, Scheduler *sch);
  ~HTTP_FindWork_prealloc() {}
  void setcache(HTTP_Cache *cache);
  Work *operator()(int fd, Work::mode m);
private:
  HTTP_FindWork_prealloc(HTTP_FindWork_prealloc const&);
  HTTP_FindWork_prealloc &operator=(HTTP_FindWork_prealloc const&);
};

#endif // HTTP_FINDWORK_PREALLOC_HPP
