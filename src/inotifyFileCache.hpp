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
public:
  class inotify_cinfo : public MIME_FileCache::MIME_cinfo
  {
    friend class inotifyFileCache;
    static int inotifyfd;
    static watchmap *wmap;
    uint32_t watchd;

  public:
    inotify_cinfo(std::string &path, size_t sz)
      : MIME_FileCache::MIME_cinfo(path, sz) {}
    ~inotify_cinfo();
  };
private:
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
