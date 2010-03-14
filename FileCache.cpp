#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h> // Just for constants
#include <sys/inotify.h>
#include <unistd.h>

#include "FileCache.hpp"
#include "logging.h"
#include "Work.hpp"

using namespace std;

FileCache *FileCache::recvinotify = NULL;
Scheduler *FileCache::sch = NULL;
Work *FileCache::wmake = NULL;

FileCache::FileCache(size_t max, Scheduler *sch, Work *wmake)
  : cur(0), max(max)
{
  /* Set up static members. Doing this from the constructor means
   * we are running through these loops for every instantiation,
   * but right now I'm only using one instantiation, so it's fine. */
  if (FileCache::recvinotify == NULL)
    FileCache::recvinotify = this;
  if (FileCache::sch == NULL)
    FileCache::sch = sch;
  if (FileCache::wmake == NULL)
    FileCache::wmake = wmake;

  if ((inotifyfd = inotify_init())==-1) {
    _LOG_CRIT("inotify_init: %m");
    exit(1);
  }
  int flags;
  if ((flags = fcntl(inotifyfd, F_GETFL))==-1) {
    _LOG_CRIT("fcntl (F_GETFL): %m");
    exit(1);
  }
  if (fcntl(inotifyfd, F_SETFL, flags | O_NONBLOCK)==-1) {
    _LOG_CRIT("fcntl (F_SETFL): %m");
    exit(1);
  }
  _LOG_INFO("inotify fd is %d", inotifyfd);
}

void FileCache::inotify_cb(uint32_t events)
{
  if (!(events & EPOLLIN)) {
    _LOG_WARNING("inotifyfd activated by epoll but not readable, ignoring");
    return;
  }
  struct inotify_event iev;
  unordered_map<uint32_t, string>::iterator it;

  FileCache *r = FileCache::recvinotify;
  
  r->clock.rdlock();
  while (true) {
    if (read(r->inotifyfd, (void *)&iev, sizeof(iev))==-1) {
      if (errno != EINTR)
	break;
    }
    else {
      string tmp = r->watchds[iev.wd];
      __sync_fetch_and_add(&r->c[tmp]->invalid, 1);
      _LOG_INFO("inotify_cb: invalidated %s in cache", tmp.c_str());
    }
  }
  r->clock.unlock();
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    sch->reschedule(wmake->getwork(r->inotifyfd, Work::read));
}

FileCache::cinfo::cinfo(size_t sz)
  : sz(sz), refcnt(1), invalid(0)
{
  buf = new char[sz];
}

FileCache::cinfo::~cinfo()
{
  delete[] buf;
}

bool FileCache::evict()
{
  string s;
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
  if ((it = c.find(s)) != c.end() && it->second->refcnt == 0) {
    _LOG_DEBUG("evict %s", s.c_str());
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
  char *ans = NULL;
  
  clock.rdlock();
  if ((it = c.find(path)) != c.end()) {
    if (__sync_fetch_and_add(&it->second->invalid, 0)!=1) {
      __sync_fetch_and_add(&it->second->refcnt, 1);
      ans = it->second->buf;
    }
    else {
      _LOG_INFO("reserve %s: in cache but invalidated", path.c_str());
    }
    clock.unlock();
    return ans;
  }
  clock.unlock();

  struct stat statbuf;
  int fd;

  // Try to stat and open file.
  if (stat(path.c_str(), &statbuf)==-1) {
    _LOG_WARNING("reserve %s stat: %m", path.c_str());
    return NULL;
  }
  else if (!S_ISREG(statbuf.st_mode)) {
    _LOG_WARNING("reserve %s: not a regular file", path.c_str());
    return NULL;
  }
  else if (fd = open(path.c_str(), O_RDONLY) ==-1) {
    _LOG_WARNING("reserve %s: open: %m", path.c_str());
    return NULL;
  }

  sz = statbuf.st_size;

 reserve_tryagain:
  
  // Not enough room in the cache?
  if (__sync_add_and_fetch(&cur, sz) > max) {
    __sync_sub_and_fetch(&cur, sz);
    _LOG_WARNING(
		 "reserve %s: not enough room in cache"
		 "(cur ~%d, max %d, need %d)"
		 "--attempting to evict",
		 path.c_str(), cur, max, sz);
    // If we were able to evict something try again.
    if (evict()) {
      goto reserve_tryagain;
    }
    // Otherwise just give up.
    else {
      _LOG_ERR("reserve %s: nothing to evict, giving up", path.c_str());
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
    _LOG_ERR("reserve %s: allocation of %d bytes failed, giving up",
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
      _LOG_DEBUG("reserve %s: read: %m", path.c_str());
      continue;
    }
    else
      break;
  }
  // Some other kind of error; start over.
  if (nread == -1) {
    _LOG_WARNING("reserve %s: read: %m, starting read over", path.c_str());
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
    _LOG_DEBUG("reserve %s: file appeared in cache"
	       " while we were reading from disk", path.c_str());
    delete tmp;
    __sync_sub_and_fetch(&cur, sz);
    it->second->refcnt++;
    sz = it->second->sz;
    ctmp = it->second->buf;
  }
  else {
    _LOG_DEBUG("reserve %s: is now in cache", path.c_str());
    uint32_t watchd;
    if ((watchd = inotify_add_watch(inotifyfd, path.c_str(), IN_MODIFY))==-1) {
	_LOG_WARNING("reserve %s: inotifyfd: %m, so not watching",
		     path.c_str());
    }
    else {
      watchds.insert(make_pair(watchd, path));
    }
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
    _LOG_DEBUG("release %s: no one else is using, putting on evict list",
	       path.c_str());
    toevict.enq(it->first);
  }
}
