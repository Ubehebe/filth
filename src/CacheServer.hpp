#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include <stdlib.h>
#include <unistd.h>

#include "CacheFindWork.hpp"
#include "inotifyFileCache.hpp"
#include "Server.hpp"

class CacheServer : public Server
{
  CacheFindWork fwork;
  inotifyFileCache cache;
public:
  CacheServer(char const *sockname,
	      char const *mount,
	      int nworkers, 
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq)
    : Server(AF_LOCAL, fwork, mount, sockname, nworkers, listenq),
      fwork(req_prealloc_MB * (1<<20), sch, cache),
      cache(cacheszMB * (1<<20), fwork, sch) {}
};


#endif // CACHE_SERVER_HPP
