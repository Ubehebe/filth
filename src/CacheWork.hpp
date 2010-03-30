#ifndef CACHE_WORK_HPP
#define CACHE_WORK_HPP

#include <sstream>
#include <string>

#include "FileCache.hpp"
#include "FindWork_prealloc.hpp"
#include "LockFreeQueue.hpp"
#include "Scheduler.hpp"
#include "ServerErrs.hpp"
#include "Work.hpp"

class CacheWork : public Work
{
  friend class CacheFindWork;
  friend class FindWork_prealloc<CacheWork>;
  CacheWork(CacheWork const &);
  CacheWork &operator=(CacheWork const &);

  static LockFreeQueue<void *> store;
  static size_t const rdbufsz = 1<<7;
  static Scheduler *sch;
  static FileCache *cache;
  static Workmap *st;

  std::stringstream inbuf;
  char rdbuf[rdbufsz];
  std::string path, statln;
  size_t resourcesz, outsz;
  char *resource, *out;
  bool path_written;
  
public:
  CacheWork(int fd, Work::mode m);
  ~CacheWork();
  void operator()();
  void *operator new(size_t sz);
  void operator delete(void *ptr);
};

#endif // CACHE_WORK_HPP
