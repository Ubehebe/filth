#ifndef INOTIFY_FILECACHE_HPP
#define INOTIFY_FILECACHE_HPP

#include <stdint.h>
#include <unordered_map>

#include "Callback.hpp"
#include "FileCache.hpp"
#include "Scheduler.hpp"

class inotifyFileCache : public Callback, public FileCache
{
  typedef std::unordered_map<uint32_t, std::string> watchmap;
  watchmap wmap;
  Scheduler &sch;
  int inotifyfd;
  bool evict();
public:
  inotifyFileCache(size_t max, FindWork &fwork, Scheduler &sch);
  char *reserve(std::string &path, size_t &sz);
  void operator()();
};

#endif // INOTIFY_FILECACHE
