#include <fcntl.h>
#include <stdlib.h>

#include "FileCache.hpp"
#include "logging.h"

using namespace std;

FileCache::cinfo::cinfo(size_t sz)
  : sz(sz), refcnt(1)
{
  buf = new char[sz];
}

FileCache::cinfo::~cinfo()
{
  delete[] buf;
}

bool FileCache::evict()
{
  bool ans = false;
  string *s;
  unordered_map<string, cinfo *>::iterator it;

  // Nothing to evict.
  if (!toevict.nowait_deq(s))
    return false;

  /* A file is put on the eviction list whenever its reference count falls to
   * 0; it is not automatically removed from the list when its reference count
   * increases. Thus, one file may appear more than once on the list, and files
   * with positive reference counts may appear on the list. Therefore any file
   * that is already gone from the cache, and any file that has a positive
   * reference count, should just be ignored. */
  clock.wrlock();
  if ((it = c.find(*s)) != c.end() && it->second->refcnt == 0) {
    ans = true;
    __sync_sub_and_fetch(&cur, it->second->sz);
    delete it->second;
    c.erase(it);
  }
  clock.unlock();
  return ans;
}

/* N.B. this function passes path directly to the kernel.
 * If the application requires path to be in a more restrictive form than
 * the kernel does (e.g., no ".." to escape the current directory), it should
 * already have checked for it. */
char *FileCache::reserve(std::string &path, size_t &sz)
{
  unordered_map<std::string, cinfo *>::iterator it;
  
  clock.rdlock();
  if ((it = c.find(path)) != c.end()) {
    __sync_fetch_and_add(&it->second->refcnt, 1);
    clock.unlock();
    return it->second->buf;
  }
  clock.unlock();

  struct stat statbuf;
  int fd;

  // Try to stat and open file.
  if (stat(path.c_str(), &statbuf)==-1
      || !S_ISREG(statbuf.st_mode)
      || (fd = open(path.c_str(), O_RDONLY)) ==-1)
    return NULL;

  sz = statbuf.st_size;

 reserve_tryagain:
  
  // Not enough room in the cache?
  if (__sync_add_and_fetch(&cur, sz) > max) {
    __sync_sub_and_fetch(&cur, sz);
    // If we were able to evict something try again.
    if (evict()) {
      goto reserve_tryagain;
    }
    // Otherwise just give up.
    else {
      close(fd);
      return NULL;
    }
  }

  cinfo *tmp;
  try {
    tmp = new cinfo(sz);
  }
  // If we weren't able to get that much memory from the OS, give up.
  catch (bad_alloc) {
    __sync_sub_and_fetch(&cur, sz);
    close(fd);
    return NULL;
  }

  char *ctmp = tmp->buf;
  size_t toread = sz;
  ssize_t nread;

  /* Get the file into memory with an old-fashioned blocking read.
   * TODO: replace with asynchronous I/O? */
  while (true) {
    if ((nread = read(fd, (void *) ctmp, toread)) > 0) {
      toread -= nread;
      ctmp += nread;
    }
    /* Interrupted by a system call. My current feeling is that the guys who
     * work with the cache (i.e., workers) should not also be the guys who
     * handle signals (i.e., schedulers or event managers), so that signals
     * should be masked here. But let's check anyway in case there is a
     * reason for such a configuration. */
    else if (nread == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  // Some other kind of error; start over.
  if (nread == -1) {
    _LOG_WARNING("read: %m");
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    goto reserve_tryagain;
  }
  close(fd);
  
  clock.wrlock();
  /* While we were getting the file into memory, someone else
   * put it in cache! So we don't need our copy anymore. Wasteful
   * but simple. Note that in this case the struct stat returned to the
   * caller could be stale. Could be important if, e.g., we want
   * modification times. */
  if ((it = c.find(path)) != c.end()) {
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    it->second->refcnt++;
    sz = it->second->sz;
    ctmp = it->second->buf;
  }
  else {
    c[path] = tmp;
    ctmp = tmp->buf;
  }
  clock.unlock();
  return ctmp;
}

void FileCache::release(std::string &path)
{
  unordered_map<string, cinfo *>::iterator it;
  bool doenq;
  clock.rdlock();
  // The first half should always be true...right?
  doenq = ((it = c.find(path)) != c.end()
	   && __sync_sub_and_fetch(&it->second->refcnt, 1)==0);
  clock.unlock();
  if (doenq)
    toevict.enq(const_cast<string *>(&it->first));
}
