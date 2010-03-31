#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CacheServer.hpp"
#include "logging.h"
#include "ThreadPool.hpp"

CacheServer *CacheServer::theserver = NULL;

CacheServer::CacheServer(
			 char const *sockname,
			 char const *mount,
			 int nworkers,
			 size_t cachesz,
			 int listenq,
			 int sigflush,
			 int sigdl_int,
			 int sigdl_ext)
  : Server(AF_LOCAL, fwork, mount, sockname, nworkers, listenq, NULL, this, 
	   this, NULL, sigdl_int, sigdl_ext),
    cachesz(cachesz), sigflush(sigflush), perform_startup(true)
{
  struct stat buf;
  if (stat(sockname, &buf)!= 0 && errno != ENOENT) {
    _LOG_FATAL("%s exists, don't want to unlink it", sockname);
    exit(1);
  }

#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
  theserver = this;
}

void CacheServer::operator()()
{
  if (perform_startup) {
    fwork = new CacheFindWork(*sch);
    /* Need to do this now because the inotifyFileCache constructor registers
     * callbacks with the scheduler. */
    sch->setfwork(fwork);
    cache = new FileCache(cachesz, *fwork);
    fwork->setcache(*cache);
    sch->push_sighandler(sigflush, flush);
  } else {
    /* Delete the work stuff before the cache because when a work object
     * is deleted, it tries to remove itself from the cache. */
    delete fwork;
    delete cache;
  }
  perform_startup = !perform_startup;
}

CacheServer::~CacheServer()
{
  _SHOW_STAT(flushes);
}

void CacheServer::flush(int ignore)
{
  theserver->cache->flush();
  // ???  theserver->sch->halt();
  _INC_STAT(theserver->flushes);
}

