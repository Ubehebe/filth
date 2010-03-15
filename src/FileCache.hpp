#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <list>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "Callback.hpp"
#include "LockedQueue.hpp"
#include "Locks.hpp"
#include "Scheduler.hpp"
#include "FindWork.hpp"

/* Simple file cache. The biggest inefficiency is that it calls new for
 * each allocation, rather than malloc'ing all the memory at the outset
 * and managing it itself. I did this because I do not know how to design
 * a concurrent memory allocator; all my ideas involve walking a free list,
 * and I don't know how to make that thread-safe without locking the whole
 * list. TODO: learn more about lock-free lists! */
class FileCache : public Callback
{
  // No copying, no assigning.
  FileCache(FileCache const&);
  FileCache &operator=(FileCache const&);

  struct cinfo
  {
    char *buf;
    int refcnt;
    int invalid;
    size_t sz;
    cinfo(size_t sz);
    ~cinfo();
  };
  std::unordered_map<std::string, cinfo *> c;
  RWLock clock; // watchds also uses this.
  LockedQueue<std::string> toevict;
  size_t cur, max;

  bool evict();

  std::unordered_map<uint32_t, std::string> watchds;

  /* I didn't intend to have more than one instantiation of a file
   * cache at a time, but if we do, they should share all this. */
  Scheduler &sch;
  FindWork &fwork;

public:
  FileCache(size_t max, Scheduler &sch, FindWork &fwork);

  /* This function should search the cache and return the file if it's there.
   * If it's not, it should read it in from disk, then return it.
   * If it's unable to get it from disk (e.g. it doesn't exist, or allocation
   * error), it should return NULL. */
  char *reserve(std::string &path, size_t &sz);
  void release(std::string &path);
  void operator()();
  /* This might as well be public since an accessor could do anything to the
   * descriptor. */
  int inotifyfd;
};

#endif // FILECACHE_HPP
