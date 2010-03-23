#include "HTTP_Server.hpp"
#include "SigThread.hpp"

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
			 int sigdeadlock_internal,
			 int sigdeadlock_external)
  : Server((ipv6) ? AF_INET6 : AF_INET, fwork, mount, portno, nworkers, listenq,
	   sigdeadlock_internal, sigdeadlock_external, ifnam),
    fwork(req_prealloc_MB * (1<<20), sch, cache),
    cache(cacheszMB * (1<<20), fwork, sch)
{
#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
  theserver = this;
  sch.push_sighandler(sigflush, flush);
}

HTTP_Server::~HTTP_Server()
{
  _SHOW_STAT(flushes);
}

void HTTP_Server::flush(int ignore)
{
  // Hmm...
  theserver->cache.flush();
  theserver->sch.poisonpill();
  _INC_STAT(theserver->flushes);
}
