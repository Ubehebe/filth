#include "HTTP_FindWork_prealloc.hpp"

HTTP_FindWork_prealloc::HTTP_FindWork_prealloc(size_t req_prealloc, Scheduler *sch)
  : FindWork_prealloc<HTTP_Server_Work_big>(req_prealloc)
{
  HTTP_Server_Work_big::setsch(sch);
  HTTP_Server_Work_big::st = &st;
}

void HTTP_FindWork_prealloc::setcache(HTTP_Cache *cache)
{
  HTTP_Server_Work_big::cache = cache;
}
