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
	   sigdl_int, sigdl_ext, ifnam, this, this),
    req_prealloc_MB(req_prealloc_MB), cacheszMB(cacheszMB), sigflush(sigflush),
    sigdl_ext(sigdl_ext), perform_startup(true)
{
#ifdef _COLLECT_STATS
  flushes = 0;
#endif // _COLLECT_STATS
  theserver = this;
}

void HTTP_Server::operator()()
{
  if (perform_startup) {
    fwork = new HTTP_FindWork(req_prealloc_MB * (1<<20), *sch, *cache); // yikes
    /* Need to do this now because the inotifyFileCache constructor registers
     * callbacks with the scheduler. */
    sch->setfwork(fwork);
    cache = new inotifyFileCache(cacheszMB * (1<<20), *fwork, *sch);
    sch->push_sighandler(sigflush, flush);
    sch->push_sighandler(SIGINT, halt); 
    sch->push_sighandler(SIGTERM, halt);
    sch->push_sighandler(sigdl_ext, UNSAFE_emerg_yank_wrapper);
  } else {
    delete cache;
    delete fwork;
  }
  perform_startup = !perform_startup;
}

HTTP_Server::~HTTP_Server()
{
  _SHOW_STAT(flushes);
}

void HTTP_Server::flush(int ignore)
{
  // Hmm...
  theserver->cache->flush();
  theserver->sch->halt();
  _INC_STAT(theserver->flushes);
}

void HTTP_Server::halt(int ignore)
{
  theserver->doserve = false;
  theserver->sch->halt();
}

void HTTP_Server::UNSAFE_emerg_yank_wrapper(int ignore)
{
  ThreadPool<Worker>::UNSAFE_emerg_yank();
  theserver->sch->halt();
}
