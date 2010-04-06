#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <list>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "Cache.hpp"
#include "Callback.hpp"
#include "Compressor.hpp"
#include "HTTP_cmdline.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_FindWork.hpp"
#include "logging.h"
#include "Magic.hpp"
#include "Server.hpp"
#include "ServerErrs.hpp"
#include "Time.hpp"
#include "Workmap.hpp"

class HTTP_Server : public Server, public Callback
{
public:
  HTTP_Server(char const *portno,
	      char const *ifnam,
	      char const *mount,
	      int nworkers,
	      bool ipv6,
	      size_t cacheszMB,
	      size_t req_prealloc_MB,
	      int listenq,
	      int sigflush,
	      int sigdl_int,
	      int sigdl_ext);
  ~HTTP_Server();
  void operator()(); // Callback into Server's main loop.
  #ifdef _COLLECT_STATS
  uint32_t flushes;
#endif // _COLLECT_STATS

private:
  HTTP_Server(HTTP_Server const&);
  HTTP_Server &operator=(HTTP_Server const&);

  // These are pointers because they can get torn down and rebuilt.
  HTTP_FindWork *fwork;
  HTTP_Cache *cache;
  Time *date;
  Compressor *compress;
  Magic *MIME;

  static HTTP_Server *theserver; // For non-signalfd-based signal handling
  static void flush(int ignore=-1);

  /* Multiplex callbacks: the server wants one for startup and one for shutdown,
   * but since we're building operator() right into the server object, we can
   * have only one. Use this bit to tell which one we're supposed to do. */
  bool perform_startup;

  size_t req_prealloc_MB, cacheszMB;
  int sigflush;


};

#endif // HTTP_SERVER_HPP
