#include <fcntl.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "inotifyFileCache.hpp"
#include "logging.h"
#include "Work.hpp"

using namespace std;

int inotifyFileCache::inotify_cinfo::inotifyfd = -1;
inotifyFileCache::watchmap *inotifyFileCache::inotify_cinfo::wmap = NULL;

FileCache::cinfo *inotifyFileCache::mkcinfo(string &path, size_t sz)
{
  // If this fails, it will be caught by base class.
  inotify_cinfo *ans = new inotify_cinfo(sz);
  if ((ans->watchd = inotify_add_watch(inotifyfd, path.c_str(),
				       IN_MODIFY|IN_DELETE_SELF|IN_MOVE_SELF))
      ==-1) {
    _LOG_INFO("inotify_add_watch: %m, so not watching %s", path.c_str());
  }
  else {
    clock.wrlock();
    wmap[ans->watchd] = path;
    clock.unlock();
  }
  return ans;
}

inotifyFileCache::inotify_cinfo::~inotify_cinfo()
{
  // N.B. we have the writer lock; look at where delete occurs in base class.
  /* According to the man pages, inotify_rm_watch causes inotifyfd to
   * become readable, and upon a read we will get watchd back with
   * the IN_IGNORED bit on. See corresponding code in operator(). */
  if (inotify_rm_watch(inotifyfd, watchd)==-1)
    _LOG_INFO("inotify_rm_watch watchd %d: %m, continuing", watchd);
  wmap->erase(watchd);
}

inotifyFileCache::inotifyFileCache(size_t max, FindWork &fwork, Scheduler &sch)
  : FileCache(max, fwork), sch(sch)
{
  if ((inotifyfd = inotify_init())==-1) {
    _LOG_FATAL("inotify_init: %m");
    exit(1);
  }
  int flags;
  if ((flags = fcntl(inotifyfd, F_GETFL))==-1) {
    _LOG_FATAL("fcntl (F_GETFL): %m");
    exit(1);
  }
  if (fcntl(inotifyfd, F_SETFL, flags | O_NONBLOCK)==-1) {
    _LOG_FATAL("fcntl (F_SETFL): %m");
    exit(1);
  }
  _LOG_INFO("inotify fd is %d", inotifyfd);

  inotify_cinfo::inotifyfd = inotifyfd;
  inotify_cinfo::wmap = &wmap;
  
  sch.registercb(inotifyfd, this, Work::read);
}

inotifyFileCache::~inotifyFileCache()
{
  flush();
  /* flush does a _SYNC_INC_STAT(flushes), but if we're here the next
   * destructor will be FileCache::~FileCache, which will also do a
   * _SYNC_INC_STAT(flushes) although the cache has just been flushed. */
  _SYNC_DEC_STAT(flushes);
}

void inotifyFileCache::operator()()
{
  struct inotify_event iev;
  cache::iterator cit;
  watchmap::iterator wit;

  clock.rdlock();
  while (true) {
    if (read(inotifyfd, (void *)&iev, sizeof(iev))==-1) {
      if (errno != EINTR)
	break;
    }
    else if ((wit = wmap.find(iev.wd)) != wmap.end()) {
      // We don't check iev.mask flags here, but we could.
      if ((cit = c.find(wit->second)) != c.end()) {
	__sync_fetch_and_add(&cit->second->invalid, 1);
	_SYNC_INC_STAT(invalidations);
	_LOG_DEBUG("invalidated %s", wit->second.c_str());
      }
      else {
	_LOG_INFO("unexpected: %s modified on disk, but not found in cache",
		  (wit->second).c_str());
      }
    }
    /* IN_IGNORED is set whenever a watch is explicitly removed
     * (inotify_rm_watch) or automatically removed (someone did an rm).
     * The second case is already handled, since we set IN_DELETE_SELF
     * in the call to inotify_add_watch. The first case happens in the
     * inotify_cinfo destructor, at which point the watch descriptor is also
     * removed from the watch map. Thus we just ignore this. */
    else if (iev.mask & IN_IGNORED) {
      continue;
    }
    else {
      _LOG_INFO("unexpected: unknown watch descriptor %d, ignoring", iev.wd);
    }
  }
  clock.unlock();
  if (errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
    sch.reschedule(fwork(inotifyfd, Work::read));
  else {
    _LOG_FATAL("read: %m");
    exit(1);
  }
}
