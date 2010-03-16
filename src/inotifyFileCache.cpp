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
  if ((ans->watchd = inotify_add_watch(inotifyfd, path.c_str(), IN_MODIFY))
      ==-1) {
    _LOG_INFO("inotify_add_watch: %m, so not watching %s", path.c_str());
  }
  else {
    clock.wrlock();
    wmap[ans->watchd] = path;
    clock.unlock();
  }
}

inotifyFileCache::inotify_cinfo::~inotify_cinfo()
{
  // N.B. we have the writer lock; look at where delete occurs in base class.
  if (inotify_rm_watch(inotifyfd, watchd)==-1)
    _LOG_INFO("inotify_rm_watch watchd %d: %m, continuing", watchd);
  wmap->erase(watchd);
  // Need to explicitly call base destructor??
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
      if ((cit = c.find(wit->second)) != c.end()) {
	__sync_fetch_and_add(&cit->second->invalid, 1);
	_LOG_INFO("%s invalidated", wit->second.c_str());
      }
      else {
	_LOG_INFO("%s modified on disk, but not found in cache",
		  (wit->second).c_str());
      }
    }
    else {
      _LOG_INFO("received unknown watch descriptor %d", iev.wd);
    }
  }
  clock.unlock();
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    sch.reschedule(fwork(inotifyfd, Work::read));
  else {
    _LOG_FATAL("read: %m");
    exit(1);
  }
}
