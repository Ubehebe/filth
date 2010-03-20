#ifndef CACHE_FINDWORK_HPP
#define CACHE_FINDWORK_HPP

#include "CacheWork.hpp"
#include "FindWork_prealloc.hpp"

class CacheFindWork : public FindWork_prealloc<CacheWork>
{
  CacheFindWork(CacheFindWork const &);
  CacheFindWork &operator=(CacheFindWork const &);
public:
  CacheFindWork(size_t prealloc_MB, Scheduler &sch, FileCache &cache);
  Work *operator()(int fd, Work::mode m);
};

#endif // CACHE_FINDWORK_HPP
