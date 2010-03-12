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
  string *s;
  unordered_map<string, cinfo *>::iterator it;

 evict_tryagain:

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
    _LOG_DEBUG("FileCache::evict %s", s->c_str());
    __sync_sub_and_fetch(&cur, it->second->sz);
    delete it->second;
    c.erase(it);
    clock.unlock();
    return true;
  }
  else {
    clock.unlock();
    goto evict_tryagain;
  }
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
  if (stat(path.c_str(), &statbuf)==-1) {
    _LOG_WARNING("FileCache::reserve %s stat: %m", path.c_str());
    return NULL;
  }
  else if (!S_ISREG(statbuf.st_mode)) {
    _LOG_WARNING("FileCache::reserve %s: not a regular file", path.c_str());
    return NULL;
  }
  else if (fd = open(path.c_str(), O_RDONLY) ==-1) {
    _LOG_WARNING("FileCache::reserve %s: open: %m", path.c_str());
    return NULL;
  }

  sz = statbuf.st_size;

 reserve_tryagain:
  
  // Not enough room in the cache?
  if (__sync_add_and_fetch(&cur, sz) > max) {
    __sync_sub_and_fetch(&cur, sz);
    _LOG_WARNING(
		 "FileCache::reserve %s: not enough room in cache"
		 "(cur ~%d, max %d, need %d)"
		 "--attempting to evict",
		 path.c_str(), cur, max, sz);
    // If we were able to evict something try again.
    if (evict()) {
      goto reserve_tryagain;
    }
    // Otherwise just give up.
    else {
      _LOG_ERR("FileCache::reserve %s: nothing to evict, giving up",
	       path.c_str());
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
    _LOG_ERR("FileCache::reserve %s: allocation of %d bytes failed, giving up",
	     path.c_str(), sz);
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
    else if (nread == -1 && errno == EINTR) {
      _LOG_DEBUG("FileCache::reserve %s: read: %m", path.c_str());
      continue;
    }
    else
      break;
  }
  // Some other kind of error; start over.
  if (nread == -1) {
    _LOG_WARNING("FileCache::reserve %s: read: %m, starting read over",
		 path.c_str());
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    goto reserve_tryagain;
  }
  close(fd);
  
  clock.wrlock();
  /* While we were getting the file into memory, someone else
   * put it in cache! So we don't need our copy anymore. Wasteful
   * but simple. */
  if ((it = c.find(path)) != c.end()) {
    _LOG_DEBUG("FileCache::reserve %s:"
	       "file appeared in cache while we were reading from disk",
	       path.c_str());
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    it->second->refcnt++;
    sz = it->second->sz;
    ctmp = it->second->buf;
  }
  else {
    _LOG_DEBUG("FileCache::reserve %s: is now in cache", path.c_str());
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
  if (doenq) {
    _LOG_DEBUG("FileCache::release %s:"
	       "no one else is using, putting on evict list", path.c_str());
    toevict.enq(const_cast<string *>(&it->first));
  }
}
