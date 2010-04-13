#include "HTTP_Server.hpp"
#include "ThreadPool.hpp"

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
			 int sigdl_ext,
			 int tcp_keepalive_intvl,
			 int tcp_keepalive_probes,
			 int tcp_keepalive_time)
  : Server((ipv6) ? AF_INET6 : AF_INET, fwork, wfact, mount, portno, nworkers, listenq,
	   ifnam, NULL, sigdl_int, sigdl_ext, tcp_keepalive_intvl,
	   tcp_keepalive_probes, tcp_keepalive_time),
    req_prealloc_MB(req_prealloc_MB), cacheszMB(cacheszMB), sigflush(sigflush)
{
#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
}

void HTTP_Server::onstartup()
{
  fwork = new HTTP_FindWork(req_prealloc_MB * (1<<20), sch);
  sch->setfwork(fwork);
  cache = new HTTP_Cache(cacheszMB * (1<<20));
  fwork->setcache(cache);
  sch->push_sighandler(sigflush, flush);
}

void HTTP_Server::onshutdown()
{
  /* Delete the work stuff before the cache because when a work object
   * is deleted, it tries to un-reserve its resources in the cache. */
  delete fwork;
  delete cache;
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
