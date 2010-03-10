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

/* Simple file cache. The biggest inefficiency is that it calls new for
 * each allocation, rather than malloc'ing all the memory at the outset
 * and managing it itself. I did this because I do not know how to design
 * a concurrent memory allocator; all my ideas involve walking a free list,
 * and I don't know how to make that thread-safe without locking the whole
 * list. TODO: learn more about lock-free lists! */
class FileCache
{
  struct cinfo
  {
    char *buf;
    int refcnt;
    size_t sz;
    cinfo(size_t sz);
    ~cinfo();
  };
  std::unordered_map<std::string, cinfo *> c;
  LockedQueue<std::string const *> toevict;
  size_t cur, max;
  RWLock lock;

  bool evict();

public:
  FileCache(size_t max) : cur(0), max(max) {}

  /* This function should search the cache and return the file if it's there.
   * If it's not, it should read it in from disk, then return it.
   * If it's unable to get it from disk (e.g. it doesn't exist, or allocation
   * error), it should return NULL.
   *
   * Since reserve may perform a stat system call, we allow the result
   * to optionally be returned in the second argument. This could be useful
   * when e.g. we don't know whether a file is static (=regular file) or
   * dynamic (=domain socket). We check the cache--if it's there, it's static.
   * If it's not there, it might be dynamic, or it might not exist. We can
   * use statbuf to tell the difference. */
  char *reserve(std::string &filename, struct stat *statbuf=NULL);
  void release(std::string &filename);
};

#endif // FILECACHE_HPP
