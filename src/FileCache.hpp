#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <list>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "LockFreeQueue.hpp"
#include "Locks.hpp"
#include "logging.h"
#include "FindWork.hpp"

/* Simple file cache. The biggest inefficiency is that it calls new for
 * each allocation, rather than malloc'ing all the memory at the outset
 * and managing it itself.
 *
 * NOTE:
 * I designed this cache to be hard-wired into a server. An alternative is to
 * regard the cache itself as a kind of server; the "main" server parses full
 * requests, and dispatches requests to cacheable resources to the cache
 * server. The parsing done by the cache server would then be very simple.
 * This would also make the cache much easier to test separately, since
 * it is exposed as a process of its own. To this end, I designed a cache
 * server class, although for my HTTP server I kept the cache hard-wired
 * for performance reasons.
 *
 * If you are running the standalone cache server tests and seeing problems,
 * I would recommend first looking at the standalone cache server classes,
 * not the file cache classes. The protocol governing the text read and written
 * by the standalone cache server is quite ad hoc; for example to indicate
 * a failure, the server just echoes back the request line and then closes
 * the connection. I've wasted more than one day "debugging" what looked
 * like inconsistencies in the file cache, when in reality it was just the test
 * suite (client) misinterpreting the server's correct results. */
class FileCache
{
  FileCache(FileCache const&);
  FileCache &operator=(FileCache const&);

protected:
  /* Note that this base class has the concept of a cache entry being
   * invalid, but no mechanism to actually invalidate cache entries.
   * We leave that up to derived classes. */
  struct cinfo
  {
    char *buf;
    int refcnt;
    int invalid; // Rather than bool so we can use atomic builtins.
    size_t sz;
    cinfo(size_t sz);
    virtual ~cinfo();
  };
#ifdef _COLLECT_STATS
  uint32_t hits, misses, evictions, invalid_hits, invalidations, failures, flushes;
#endif // _COLLECT_STATS
  typedef std::unordered_map<std::string, cinfo *> cache;
  cache c;
  RWLock clock;
  LockFreeQueue<std::string> toevict;
  size_t cur, max;
  FindWork &fwork;

  bool evict();
  virtual cinfo *mkcinfo(std::string &path, size_t sz);

public:
  FileCache(size_t max, FindWork &fwork);
  ~FileCache();
  int reserve(std::string &path, char *& resource, size_t &sz);
  void release(std::string &path);
  size_t getmax() const { return max; }
  void flush();
};

#endif // FILECACHE_HPP
