#include <sys/socket.h>
#include <sys/types.h>

#include "CacheServer.hpp"

CacheServer *CacheServer::theserver = NULL;

CacheServer::CacheServer(char const *sockname, char const *mount,
			 int nworkers, size_t cacheszMB, size_t req_prealloc_MB,
			 int listenq, int sigdeadlock, int sigflush)
  : Server(AF_LOCAL, fwork, mount, sockname, nworkers, listenq),
    fwork(req_prealloc_MB * (1<<20), sch, cache),
    cache(cacheszMB * (1<<20), fwork, sch)
{
#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
  theserver = this;
  sch.push_sighandler(sigflush, flush);
  sch.push_sighandler(sigdeadlock, Thread<Worker>::emerg_cancelall);
}

CacheServer::~CacheServer()
{
  _SHOW_STAT(flushes);
}

void CacheServer::flush(int ignore)
{
  theserver->cache.flush();
  theserver->sch.poisonpill();
  _INC_STAT(theserver->flushes);
}

