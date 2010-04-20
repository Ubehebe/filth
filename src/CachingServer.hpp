#ifndef CACHING_SERVER_HPP
#define CACHING_SERVER_HPP

#include "Cache.hpp"
#include "Server.hpp"

/** \brief A generic server-with-a-cache. */
template<class _Work, class _Worker, class _CacheEntry> class CachingServer
  : public Server<_Work, _Worker>
{
public:
    /** \param domain AF_INET, AF_INET6 or AF_LOCAL
   * \param mount Directory in the file system to use as the server's root
   * directory: it should be impossible to see anything outside this
   * \param bindto for AF_INET or AF_INET6, a human-readable string 
   * representing a port number, e.g. "80". For AF_LOCAL, a filesystem path.
   * \param cacheszMB size of the cache, in MB
   * \param sigflush signal to use to flush the cache; default of -1 means
   * this capability is disabled
   * \param nworkers number of worker threads
   * \param listenq TCP listen queue size (does this have any effect for
   * AF_LOCAL? Don't know.)
   * \param preallocMB size in MB to preallocate for _Work objects
   * \param ifnam human-readable string of interface to use, e.g.
   * "eth0" (not for AF_LOCAL)
   * \param haltsigs signals that should cause the server to do an orderly
   * shutdown. If NULL, the default signals are SIGINT and SIGTERM.
   * \param sigdl_int signal to be used internally by the emergency thread
   * de-deadlock mechanism. Default of -1 means the emergency thread
   * de-deadlock mechanism is disabled.
   * \param sigdl_ext signal to be used to trigger the emergency de-deadlock
   * mechanism. Default of -1 means the emergency de-deadlock mechanism
   * is disabled.
   * \param tcp_keepalive_intvl see man 7 tcp. Default of -1 means don't
   * do anything special.
   * \param tcp_keepalive_probes see man 7 tcp. Default of -1 means don't
   * do anything special.
   * \param tcp_keepalive_time see man 7 tcp. Default of -1 means don't
   * do anything special.
   * \param untrusted_uid untrusted user id to execute as, once we are done
   * with the (possibly privileged) bind
   * \param untrusted_gid untrusted group id to execute as, once we are done
   * with the (possibly privileged) bind */
  CachingServer(
		int domain,
		char const *mount,
		char const *bindto,
		size_t cacheszMB=10, // new
		int sigflush=-1, // new
		int nworkers=10,
		int listenq=100,
		size_t preallocMB=10,
		char const *ifnam=NULL,
		sigset_t *haltsigs=NULL,
		int sigdl_int=-1,
		int sigdl_ext=-1,
		int tcp_keepalive_intvl=-1,
		int tcp_keepalive_probes=-1,
		int tcp_keepalive_time=-1,
		uid_t untrusted_uid=9999,
		gid_t untrusted_gid=9999);
  ~CachingServer() {}
  /** \brief Flush the server.
   * \param ignore Dummy variable, needed because of the restrictions
   * sigaction places on its function pointer */
  static void flush(int ignore=-1);
private:
  static CachingServer *theserver;
  Cache <std::string, _CacheEntry> *cache;
  size_t cacheszMB;
  int sigflush;
  void onstartup();
  void onshutdown();
  CachingServer(CachingServer const &);
  CachingServer &operator=(CachingServer const &);
};

template<class _Work, class _Worker, class _CacheEntry>
CachingServer<_Work, _Worker, _CacheEntry>
*CachingServer<_Work, _Worker, _CacheEntry>::theserver = NULL;

template<class _Work, class _Worker, class _CacheEntry>
void CachingServer<_Work, _Worker, _CacheEntry>::flush(int ignore)
{
  theserver->cache->flush();
  theserver->sch->halt();
}

template<class _Work, class _Worker, class _CacheEntry>
CachingServer<_Work, _Worker, _CacheEntry>::CachingServer(
							  int domain,
							  char const *mount,
							  char const *bindto,
							  size_t cacheszMB,
							  int sigflush,
							  int nworkers,
							  int listenq,
							  size_t preallocMB,
							  char const *ifnam,
							  sigset_t *haltsigs,
							  int sigdl_int,
							  int sigdl_ext,
							  int tcp_keepalive_intvl,
							  int tcp_keepalive_probes,
							  int tcp_keepalive_time,
							  uid_t untrusted_uid,
							  gid_t untrusted_gid)
  : Server<_Work, _Worker>(
			   domain,
			   mount,
			   bindto,
			   nworkers,
			   listenq,
			   preallocMB,
			   ifnam,
			   haltsigs,
			   sigdl_int,
			   sigdl_ext,
			   tcp_keepalive_intvl,
			   tcp_keepalive_probes,
			   tcp_keepalive_time,
			   untrusted_uid,
			   untrusted_gid),
    sigflush(sigflush), cacheszMB(cacheszMB)
{
  theserver = this;
}

template<class _Work, class _Worker, class _CacheEntry>
void CachingServer<_Work, _Worker, _CacheEntry>::onstartup()
{
  cache = new Cache<std::string, _CacheEntry>(cacheszMB * (1<<20));
  _Work::setcache(cache);
  Server<_Work, _Worker>::sch->push_sighandler(sigflush, flush);
}

template<class _Work, class _Worker, class _CacheEntry>
void CachingServer<_Work, _Worker, _CacheEntry>::onshutdown()
{
  delete cache;
}

#endif // CACHING_SERVER_HPP
