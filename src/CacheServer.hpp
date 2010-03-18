#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include <stdlib.h>
#include <unistd.h>

#include "CacheFindWork.hpp"
#include "FileCache.hpp"
#include "Server.hpp"

class CacheServer : public Server
{
  FileCache cache;
  CacheFindWork fwork;
public:
  CacheServer(char const *sockname,
	      char const *mount,
	      int nworkers, 
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq)
    : Server(AF_LOCAL, fwork, mount, sockname, nworkers, listenq),
      fwork(req_prealloc_MB * (1<<20), sch, cache),
      cache(cacheszMB * (1<<20), fwork)
  {
    if (chdir(mount)==-1) {
      _LOG_FATAL("chdir: %m");
      exit(1);
    }
  }
};


#endif // CACHE_SERVER_HPP
