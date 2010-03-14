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
#include "Scheduler.hpp"
#include "Work.hpp"

/* Simple file cache. The biggest inefficiency is that it calls new for
 * each allocation, rather than malloc'ing all the memory at the outset
 * and managing it itself. I did this because I do not know how to design
 * a concurrent memory allocator; all my ideas involve walking a free list,
 * and I don't know how to make that thread-safe without locking the whole
 * list. TODO: learn more about lock-free lists!
 *
 * At one point I tried pulling the "unordered map + reader/writer lock" into a
 * "LockedHash" class. The reason I stopped was because I didn't know what
 * interface to export. You cannot export an STL-like interface with locking
 * done inside the calls; if you drop the lock and return the element, the
 * element could already be gone. So we would have to export a reserve/
 * release interface with internal reference counting. This is pretty much the
 * entire FileCache class. */
class FileCache
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
  static Scheduler *sch;
  static Work *wmake;
  static FileCache *recvinotify;


public:
  FileCache(size_t max, Scheduler *sch, Work *wmake);

  /* This function should search the cache and return the file if it's there.
   * If it's not, it should read it in from disk, then return it.
   * If it's unable to get it from disk (e.g. it doesn't exist, or allocation
   * error), it should return NULL. */
  char *reserve(std::string &path, size_t &sz);
  void release(std::string &path);
  int inotifyfd;
  static void inotify_cb(uint32_t events);
};

#endif // FILECACHE_HPP
