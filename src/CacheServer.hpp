#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "CacheFindWork.hpp"
#include "FileCache.hpp"
#include "Server.hpp"

class CacheServer : public Server
{
  CacheFindWork fwork;
  FileCache cache;
public:
  CacheServer(char const *sockname,
	      char const *mount,
	      int nworkers, 
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq,
	      int sigflush);
  ~CacheServer();
  static CacheServer *theserver;
  static void flush(int ignore);
#ifdef _COLLECT_STATS
  uint32_t flushes;
#endif // _COLLECT_STATS
};


#endif // CACHE_SERVER_HPP
