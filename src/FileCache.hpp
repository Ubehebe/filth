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
  // No copying, no assigning.
  FileCache(FileCache const&);
  FileCache &operator=(FileCache const&);

protected:
  struct cinfo
  {
    char *buf;
    int refcnt;
    uint32_t invalid;
    size_t sz;
    cinfo(size_t sz);
    ~cinfo();
  };
  typedef std::unordered_map<std::string, cinfo *> cache;
  cache c;
  RWLock clock;
  LockedQueue<std::string> toevict;
  size_t cur, max;
  virtual bool evict();

  FindWork &fwork;

public:
  FileCache(size_t max, FindWork &fwork) : cur(0), max(max), fwork(fwork) {}
  virtual char *reserve(std::string &path, size_t &sz);
  virtual void release(std::string &path);
};

#endif // FILECACHE_HPP
