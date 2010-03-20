#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <list>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "HTTP_cmdline.hpp"
#include "HTTP_constants.hpp"
#include "Workmap.hpp"
#include "HTTP_FindWork.hpp"
#include "inotifyFileCache.hpp"
#include "logging.h"
#include "Server.hpp"
#include "ServerErrs.hpp"

class HTTP_Server : public Server
{
  HTTP_Server(HTTP_Server const&);
  HTTP_Server &operator=(HTTP_Server const&);

  /* The cache constructor registers its inotify fd with the scheduler using
   * fwork. fwork checks st to see if such a work object already exists, so
   * st needs to have been initialized already. */
  HTTP_FindWork fwork;
  inotifyFileCache cache;

public:
  HTTP_Server(char const *portno,
	      char const *ifnam,
	      char const *mount,
	      int nworkers,
	      bool ipv6,
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq,
	      int sigflush);
  ~HTTP_Server();
  static HTTP_Server *theserver; // For non-signalfd-based signal handling
  static void flush(int ignore);
  #ifdef _COLLECT_STATS
  uint32_t flushes;
#endif // _COLLECT_STATS
};

#endif // HTTP_SERVER_HPP
