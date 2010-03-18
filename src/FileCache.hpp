#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <list>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "LockedQueue.hpp"
#include "Locks.hpp"
#include "FindWork.hpp"

/* Simple file cache. The biggest inefficiency is that it calls new for
 * each allocation, rather than malloc'ing all the memory at the outset
 * and managing it itself. I did this because I do not know how to design
 * a concurrent memory allocator; all my ideas involve walking a free list,
 * and I don't know how to make that thread-safe without locking the whole
 * list. TODO: learn more about lock-free lists! */
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
  typedef std::unordered_map<std::string, cinfo *> cache;
  cache c;
  RWLock clock;
  LockedQueue<std::string> toevict;
  size_t cur, max;
  FindWork &fwork;

  bool evict();
  virtual cinfo *mkcinfo(std::string &path, size_t sz);

public:
  FileCache(size_t max, FindWork &fwork) : cur(0), max(max), fwork(fwork) {}
  ~FileCache() { flush(); }
  char *reserve(std::string &path, size_t &sz);
  void release(std::string &path);
  size_t getmax() const { return max; }
  void flush();
};

#endif // FILECACHE_HPP
