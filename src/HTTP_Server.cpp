#include "HTTP_Server.hpp"
#include "ThreadPool.hpp"

HTTP_Server *HTTP_Server::theserver = NULL;

HTTP_Server::HTTP_Server(char const *portno,
			 char const *ifnam,
			 char const *mount,
			 int nworkers,
			 bool ipv6,
			 size_t cacheszMB,
			 size_t req_prealloc_MB,
			 int listenq,
			 int sigflush,
			 int sigdl_int,
			 int sigdl_ext)
  : Server((ipv6) ? AF_INET6 : AF_INET, fwork, mount, portno, nworkers, listenq,
	   ifnam, this, this, NULL, sigdl_int, sigdl_ext),
    req_prealloc_MB(req_prealloc_MB), cacheszMB(cacheszMB), sigflush(sigflush),
    perform_startup(true)
{
#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
  theserver = this;
}

void HTTP_Server::operator()()
{
  if (perform_startup) {
    fwork = new HTTP_FindWork(req_prealloc_MB * (1<<20), *sch);
    /* Need to do this now because the inotifyFileCache constructor registers
     * callbacks with the scheduler. */
    sch->setfwork(fwork);
    cache = new inotifyFileCache(cacheszMB * (1<<20), *fwork, *sch);
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

HTTP_Server::~HTTP_Server()
{
  _SHOW_STAT(flushes);
}

void HTTP_Server::flush(int ignore)
{
  theserver->cache->flush();
  theserver->sch->halt();
  _INC_STAT(theserver->flushes);
}

