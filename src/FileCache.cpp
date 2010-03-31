#include <fcntl.h>
#include <list>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "FileCache.hpp"
#include "logging.h"

/* INCORRECT!
 * This works fine when the cumulative size of the files being served is
 * at most the size of the cache, but this starts returning ENOMEMs indefinitely
 * when we load the cache more. Obviously what this means is that the
 * eviction mechanism is screwed up. */

using namespace std;

FileCache::FileCache(size_t max, FindWork &fwork)
  : cur(0), max(max), fwork(fwork)
{
#ifdef _COLLECT_STATS
  hits = misses = evictions = invalidations
    = invalid_hits = failures = flushes = 0;
#endif // _COLLECT_STATS
}

FileCache::~FileCache()
{
  flush();
  _SHOW_STAT(hits);
  _SHOW_STAT(misses);
  _SHOW_STAT(evictions);
  _SHOW_STAT(invalidations);
  _SHOW_STAT(invalid_hits);
  _SHOW_STAT(failures);
  _SHOW_STAT(flushes);
}

// Looks like it could cause some headaches.
void FileCache::flush()
{
  _SYNC_INC_STAT(flushes);
  list<cinfo *> l;

  clock.wrlock();
  for (cache::iterator it = c.begin(); it != c.end(); ++it)
    l.push_back(it->second);

  for (list<cinfo *>::iterator it = l.begin(); it != l.end(); ++it)
    delete *it;

  cur = 0;
  c.clear();
  clock.unlock();
}

FileCache::cinfo::cinfo(size_t sz)
  : sz(sz), refcnt(1), invalid(0)
{
  buf = new char[sz];
}

FileCache::cinfo *FileCache::mkcinfo(string &path, size_t sz)
{
  return new cinfo(sz); // No need to store path yet
}

FileCache::cinfo::~cinfo()
{
  delete[] buf;
}

bool FileCache::evict()
{
  string s;
  cache::iterator it;

 evict_tryagain:

  // Nothing to evict.
  if (!toevict.nowait_deq(s)) {
    _LOG_DEBUG("nothing to evict");
    return false;
  }

  /* A file is put on the eviction list whenever its reference count falls to
   * 0; it is not automatically removed from the list when its reference count
   * increases. Thus, one file may appear more than once on the list, and files
   * with positive reference counts may appear on the list. Therefore any file
   * that is already gone from the cache, and any file that has a positive
   * reference count, should just be ignored. */
  clock.wrlock();
  if ((it = c.find(s)) != c.end() && it->second->refcnt == 0) {
    _SYNC_INC_STAT(evictions);
    _LOG_DEBUG("evicted %s, cur goes from %d to %d",
	       s.c_str(), cur, cur - it->second->sz);
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
 * already have checked for it.
 *
 * Return values:
 * 0: success
 * EINVAL: in cache but invalidated. (Try again soon?)
 * EISDIR: the resource is a directory
 * ESPIPE: the resource is a socket or pipe
 * ENOMEM: couldn't free up enough room in cache for resource, or
 *	couldn't get enough room for resource from the kernel
 * Also any of the open or stat errors??
 */
int FileCache::reserve(std::string &path, char *&resource, size_t &sz)
{
  cache::iterator it;
  
  clock.rdlock();
  if ((it = c.find(path)) != c.end()) {
    if (__sync_fetch_and_add(&it->second->invalid, 0)!=0) {
      _SYNC_INC_STAT(invalid_hits);
      clock.unlock();
      return EINVAL;
    }
    else {
      __sync_fetch_and_add(&it->second->refcnt, 1);
      resource = it->second->buf;
      sz = it->second->sz;
      _SYNC_INC_STAT(hits);
      clock.unlock();
      return 0;
    }
  }
  clock.unlock();

  struct stat statbuf;
  int fd;

  if (stat(path.c_str(), &statbuf)==-1) {
    _SYNC_INC_STAT(failures);
    return errno;
  }
  else if (S_ISDIR(statbuf.st_mode)) {
    _SYNC_INC_STAT(failures);
    return EISDIR;
  }
  else if (S_ISSOCK(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)) {
    _SYNC_INC_STAT(failures);
    return ESPIPE;
  }
  else if ((fd = open(path.c_str(), O_RDONLY)) ==-1) {
    _SYNC_INC_STAT(failures);
    return errno;
  }

  sz = statbuf.st_size;

  // Don't even try if it can't physically fit in cache.
  if (sz > max) {
    close(fd);
    _SYNC_INC_STAT(failures);
    return ENOMEM;
  }
  
 reserve_tryagain:
  _LOG_DEBUG("trying %s sz %d cur %d max %d", path.c_str(), sz, cur, max);
  
  // Not enough room in the cache?
  if (__sync_add_and_fetch(&cur, sz) > max) {
    __sync_sub_and_fetch(&cur, sz);
    // If we were able to evict something try again.
    if (evict()) {
      goto reserve_tryagain;
    }
    // Otherwise just give up.
    else {
      _LOG_DEBUG("reserve %s sz %d: unable to evict anything", path.c_str(), sz);
      close(fd);
      _SYNC_INC_STAT(failures);
      return ENOMEM;
    }
  }

  cinfo *tmp;
  try {
    tmp = mkcinfo(path, sz);
  }
  // If we weren't able to get that much memory from the OS, give up.
  catch (bad_alloc) {
    _LOG_INFO("whoa! bad_alloc %s sz %d", path.c_str(), sz);
    __sync_sub_and_fetch(&cur, sz);
    close(fd);
    _SYNC_INC_STAT(failures);
    return ENOMEM;
  }
  char *ctmp = tmp->buf;
  size_t toread = sz;
  ssize_t nread;

  /* Get the file into memory with an old-fashioned blocking read.
   * TODO: replace with asynchronous I/O? */
  while (toread) {
    if ((nread = ::read(fd, (void *) ctmp, toread)) > 0) {
      toread -= nread;
      ctmp += nread;
    }
    else if (nread == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  // Some other kind of error; start over.
  if (nread == -1) {
    _LOG_INFO("read %s: %m, starting read over", path.c_str());

    /* Every other time in this file we delete a cinfo object, we have the
     * writer lock, so for consistency we grab it here too. This is important
     * since the cinfo destructor is virtual and derived cinfo objects could
     * perform some cleanup that requires the writer lock, for example
     * removing an inotify watch. */
    clock.wrlock();
    delete tmp;
    clock.unlock();

    __sync_sub_and_fetch(&cur, sz);
    goto reserve_tryagain;
  }
  close(fd);
  
  clock.wrlock();
  /* While we were getting the file into memory, someone else put it in cache!
   * So we don't need our copy anymore. Wasteful but simple. */
  if ((it = c.find(path)) != c.end()) {
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    it->second->refcnt++;
    sz = it->second->sz;
    resource = it->second->buf;
    _SYNC_INC_STAT(hits);
  }
  else {
    c[path] = tmp;
    resource = tmp->buf;
    _SYNC_INC_STAT(misses);
  }
  clock.unlock();
  return 0;
}

void FileCache::release(std::string &path)
{
  cache::iterator it;
  bool doenq, doevict;
  clock.rdlock();
  doenq = ((it = c.find(path)) != c.end()
	   && __sync_sub_and_fetch(&it->second->refcnt, 1)==0);
  // If the file has been invalidated, we want to evict it ASAP.
  doevict = (it != c.end() &&
	     __sync_add_and_fetch(&it->second->invalid, 0) == 1);
  clock.unlock();
  if (doenq) {
    _LOG_DEBUG("%s goes on evict list", path.c_str());
    toevict.enq(path);
  }
  // Evicts the invalidated file, but first evicts everything else. Overkill?
  if (doevict) {
    while (evict())
      ;
  }
}
