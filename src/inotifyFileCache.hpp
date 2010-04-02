#ifndef INOTIFY_FILECACHE_HPP
#define INOTIFY_FILECACHE_HPP

#include <stdint.h>
#include <string>
#include <unordered_map>

#include "Callback.hpp"
#include "MIME_FileCache.hpp"
#include "Scheduler.hpp"

class inotifyFileCache : public Callback, public MIME_FileCache
{
  inotifyFileCache(inotifyFileCache const &);
  inotifyFileCache &operator=(inotifyFileCache const &);
  typedef std::unordered_map<uint32_t, std::string> watchmap;
  struct inotify_cinfo : MIME_FileCache::cinfo
  {
    static int inotifyfd;
    static watchmap *wmap;
    uint32_t watchd;
    inotify_cinfo(std::string &path, size_t sz)
      : MIME_FileCache::cinfo(path, sz) {}
    ~inotify_cinfo();
  }; 
  virtual FileCache::cinfo *mkcinfo(std::string &path, size_t sz);
  watchmap wmap;
  Scheduler &sch;
  int inotifyfd;
  bool evict();
public:
  inotifyFileCache(size_t max, FindWork &fwork, Scheduler &sch);
  ~inotifyFileCache();
  char *reserve(std::string &path, size_t &sz);
  void operator()();
};

#endif // INOTIFY_FILECACHE
