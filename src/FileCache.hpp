#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <algorithm>
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
 * requests, and dispatches requests for cacheable resources to the cache
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

public:
  /* Note that this base class has the concept of a cache entry being
   * invalid, but no mechanism to actually invalidate cache entries.
   * We leave that up to derived classes.
   *
   * TODO: the inheritance from cinfo is much too permissive. A client
   * who gets a cinfo object from reserve should be able to see the buffer,
   * size, and metadata defined in classes derived from cinfo (e.g. MIME
   * types), but should never be able to modify the buffer, invalidate, etc.
   * Clean this up! */
  class cinfo
  {
    friend class FileCache;
    /* I didn't know you could do forward friend declarations like this.
     * We need it because the inotify file cache has to be able to invalidate
     * cache entries. */
    friend class inotifyFileCache;
    char * const _buf;
    bool invalid;
    int refcnt;
  protected:
    void invalidate() { __sync_fetch_and_or(&invalid, true); }

  public:
    char const * const buf; // public version of _buf
    size_t const sz;
    cinfo(std::string &path, size_t sz);
    virtual ~cinfo();
  };
protected:
  virtual cinfo *mkcinfo(std::string &path, size_t sz);
  typedef std::unordered_map<std::string, cinfo *> cache;
  cache c;
  RWLock clock;
  LockFreeQueue<std::string> toevict;
  size_t cur, max;
  FindWork &fwork;
  bool evict();

public:
  FileCache(size_t max, FindWork &fwork);
  ~FileCache();
  cinfo *reserve(std::string &path, int &err);
  void release(std::string &path);
  size_t getmax() const { return max; }
  void flush();
#ifdef _COLLECT_STATS
  uint32_t hits, misses, evictions, invalid_hits, invalidations, failures, flushes;
#endif // _COLLECT_STATS
};

#endif // FILECACHE_HPP
