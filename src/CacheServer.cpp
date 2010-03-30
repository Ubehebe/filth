#include <sys/socket.h>
#include <sys/types.h>

#include "CacheServer.hpp"
#include "ThreadPool.hpp"

CacheServer *CacheServer::theserver = NULL;

CacheServer::CacheServer(
			 char const *sockname,
			 char const *mount,
			 int nworkers,
			 size_t cacheszMB,
			 int listenq,
			 int sigflush,
			 int sigdl_int,
			 int sigdl_ext)
  : Server(AF_LOCAL, fwork, mount, sockname, nworkers, listenq, NULL, this, 
	   this, NULL, sigdl_int, sigdl_ext),
    cacheszMB(cacheszMB), sigflush(sigflush), perform_startup(true)
{
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
    cache = new inotifyFileCache(cacheszMB * (1<<20), *fwork, *sch);
    fwork->setcache(*cache);
    sch->push_sighandler(sigflush, flush);
  } else {
    delete cache;
    delete fwork;
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
  _INC_STAT(theserver->flushes);
}

