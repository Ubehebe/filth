#ifndef CACHING_SERVER_HPP
#define CACHING_SERVER_HPP

#include "Cache.hpp"
#include "Server.hpp"

template<class _Work, class _Worker, class _CacheEntry> class CachingServer
  : public Server<_Work, _Worker>
{
public:
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
