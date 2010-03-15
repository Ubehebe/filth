#include <fcntl.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "inotifyFileCache.hpp"
#include "logging.h"
#include "Work.hpp"

using namespace std;

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

  // Is this gonna work? Nope!
  //  sch.registercb(inotifyfd, this, Work::read);
}

void inotifyFileCache::operator()()
{
  struct inotify_event iev;
  watchmap::iterator it;

  clock.rdlock();
  while (true) {
    if (read(inotifyfd, (void *)&iev, sizeof(iev))==-1) {
      if (errno != EINTR)
	break;
    }
    else {
      string tmp = wmap[iev.wd];
      __sync_fetch_and_add(&c[tmp]->invalid, iev.wd);
      _LOG_INFO("%s invalidated", tmp.c_str());
    }
  }
  clock.unlock();
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    sch.reschedule(fwork(inotifyfd, Work::read));
}

char *inotifyFileCache::reserve(string &path, size_t &sz)
{
  char *ans;
  uint32_t watchd;
  if ((ans = FileCache::reserve(path, sz))!=NULL) {
    if ((watchd = inotify_add_watch(inotifyfd, path.c_str(), IN_MODIFY))==-1) {
      _LOG_INFO("inotifyfd: %m, so not watching %s", path.c_str());
    }
    else {
      clock.wrlock();
      wmap[watchd] = path;
      clock.unlock();
    }
  }
  return ans;
}

bool inotifyFileCache::evict()
{
  string s;
  cache::iterator it;

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
    if (inotify_rm_watch(inotifyfd, it->second->invalid)==-1)
      _LOG_INFO("inotify_rm_watch %s: %m, continuing", s.c_str());
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

