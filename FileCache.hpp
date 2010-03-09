#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <list>
#include <string>
#include <unordered_map>

#include "LockedQueue.hpp"
#include "Locks.hpp"

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
  LockedQueue<std::string *> toevict;
  size_t cur, max;
  RWLock lock;

  bool evict();

public:
  FileCache(size_t max) : cur(0), max(max) {}

  /* This function should search the cache and return the file if it's there.
   * If it's not, it should read it in from disk, then return it.
   * If it's not on disk either, it should just return NULL. */
  char *reserve(std::string &filename);
  void release(std::string &filename);
};

#endif // FILECACHE_HPP
