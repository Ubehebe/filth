#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include "CacheFindWork.hpp"
#include "FileCache.hpp"
#include "Server.hpp"

class CacheServer : public Server
{
  FileCache cache;
  CacheFindWork fwork;
public:
  CacheServer(char const *bindto, size_t cacheszMB, size_t req_prealloc_MB,
	      int nworkers, int listenq)
    : Server(AF_LOCAL, fwork, bindto, NULL, nworkers, listenq),
      fwork(req_prealloc_MB * (1<<20), sch, cache),
      cache(cacheszMB * (1<<20), fwork)
  {}
};


#endif // CACHE_SERVER_HPP
