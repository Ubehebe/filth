#ifndef CACHE_FINDWORK_HPP
#define CACHE_FINDWORK_HPP

#include "CacheWork.hpp"
#include "FindWork.hpp"

class CacheFindWork : public FindWork
{
  CacheFindWork(CacheFindWork const &);
  CacheFindWork &operator=(CacheFindWork const &);
public:
  CacheFindWork(Scheduler &sch);
  Work *operator()(int fd, Work::mode m);
  void setcache(FileCache &cache);
};

#endif // CACHE_FINDWORK_HPP
