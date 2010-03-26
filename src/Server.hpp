#ifndef SERVER_HPP
#define SERVER_HPP

#include <unordered_map>

#include "Callback.hpp"
#include "FindWork.hpp"
#include "DoubleLockedQueue.hpp"
#include "Scheduler.hpp"
#include "Worker.hpp"

class Server
{
  Server &operator=(Server const&);
  Server(Server const&);

  // Internal setup functions for supported domains.
  void setup_AF_INET();
  void setup_AF_INET6();
  void setup_AF_LOCAL();
  void socket_bind_listen();

  int domain, listenfd, listenq, nworkers;
  char const *bindto, *ifnam;
  char *sockdir;


  /* These are hooks into beginning and end of the server's main loop. The idea
   * is that the server should tear down and rebuild all its resources
   * for each loop, because a loop corresponds to a call to ThreadPool's
   * UNSAFE_emerg_yank routine, which can leave data in really bad shape.
   * Classes derived from Server that add new resources (e.g. a cache)
   * should use these callbacks to tear down and rebuild those resources. */
  Callback *onstartup, *onshutdown;

  DoubleLockedQueue<Work *> *q; // Jobs waiting to be worked on.
  int sigdl_int, sigdl_ext;
  FindWork *fwork;

protected:
  Scheduler *sch;
  bool doserve;

public:

  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(
	 int domain,
	 FindWork *fwork,
	 char const *mount,
	 char const *bindto,
	 int nworkers,
	 int listenq,
	 int sigdl_int,
	 int sigdl_ext,
	 char const *ifname=NULL,
	 Callback *onstartup=NULL,
	 Callback *onshutdown=NULL);
  ~Server();
  void serve();
};

#endif // SERVER_HPP
