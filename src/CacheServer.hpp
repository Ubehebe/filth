#ifndef CACHE_SERVER_HPP
#define CACHE_SERVER_HPP

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "CacheFindWork.hpp"
#include "Callback.hpp"
#include "inotifyFileCache.hpp"
#include "Server.hpp"

class CacheServer : public Server, public Callback
{
  // Pointers because they can be torn down and rebuilt.
  CacheFindWork *fwork;
  inotifyFileCache *cache;
  bool perform_startup; // Multiplex operator()
  size_t cacheszMB;
  int sigflush;
public:
  CacheServer(char const *sockname,
	      char const *mount,
	      int nworkers, 
	      size_t cacheszMB,
	      int listenq,
	      int sigflush,
	      int sigdl_int,
	      int sigdl_ext);
  ~CacheServer();
  static CacheServer *theserver;
  static void flush(int ignore);
  void operator()();
#ifdef _COLLECT_STATS
  uint32_t flushes;
#endif // _COLLECT_STATS
};


#endif // CACHE_SERVER_HPP
