#ifndef INOTIFY_FILECACHE_HPP
#define INOTIFY_FILECACHE_HPP

#include <stdint.h>
#include <string>
#include <unordered_map>

#include "Callback.hpp"
#include "FileCache.hpp"
#include "Scheduler.hpp"

class inotifyFileCache : public Callback, public FileCache
{
  inotifyFileCache(inotifyFileCache const &);
  inotifyFileCache &operator=(inotifyFileCache const &);
  typedef std::unordered_map<uint32_t, std::string> watchmap;
  struct inotify_cinfo : FileCache::cinfo
  {
    static int inotifyfd;
    static watchmap *wmap;
    uint32_t watchd;
    inotify_cinfo(size_t sz) : FileCache::cinfo(sz) {}
    ~inotify_cinfo();
  };
  watchmap wmap;
  Scheduler &sch;
  int inotifyfd;
  bool evict();
  cinfo *mkcinfo(std::string &path, size_t sz);
public:
  inotifyFileCache(size_t max, FindWork &fwork, Scheduler &sch);
  char *reserve(std::string &path, size_t &sz);
  void operator()();
};

#endif // INOTIFY_FILECACHE
