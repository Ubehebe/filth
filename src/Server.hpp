#ifndef SERVER_HPP
#define SERVER_HPP

#include <list>
#include <sys/types.h>
#include <unordered_map>

#include "Callback.hpp"
#include "FindWork.hpp"
#include "LockFreeQueue.hpp"
#include "Locks.hpp"
#include "Scheduler.hpp"
#include "ThreadPool.hpp"
#include "Worker.hpp"

class Server
{
public:

  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(
	 int domain,
	 FindWork *fwork,
	 Factory<Worker> &wfact,
	 char const *mount,
	 char const *bindto,
	 int nworkers,
	 int listenq,
	 char const *ifnam=NULL,
	 Callback *onstartup=NULL,
	 Callback *onshutdown=NULL,
	 sigset_t *haltsigs=NULL,
	 int sigdl_int=-1,
	 int sigdl_ext=-1,
	 int tcp_keepalive_intvl=-1,
	 int tcp_keepalive_probes=-1,
	 int tcp_keepalive_time=-1);
  ~Server();
  void serve();
  void doserve(bool doserve) { _doserve = doserve; }
  static void halt(int ignore=-1);
  static void UNSAFE_emerg_yank_wrapper(int ignore=-1);

private:
  Server &operator=(Server const&);
  Server(Server const&);
  Factory<Worker> &wfact;

  // Internal setup functions for supported domains.
  void setup_AF_INET();
  void setup_AF_INET6();
  void setup_AF_LOCAL();
  void socket_bind_listen();

  int domain, listenfd, listenq, nworkers;

  int tcp_keepalive_intvl, tcp_keepalive_probes, tcp_keepalive_time;

  char const *bindto, *ifnam;



  /* A server bound to a socket in the filesystem needs to remember both
   * where it is bound (sockdir) and the subtree of the filesystem it considers
   * root (mntdir). This is because if the server does a soft reboot,
   * it will have to chdir to sockdir, bind the socket there, then chdir to
   * mntdir to service requests from there. */
  char *sockdir; // not const because it uses get_current_dir_name
  char const *mntdir;

  /* These are hooks into beginning and end of the server's main loop. The idea
   * is that the server should tear down and rebuild all its resources
   * for each loop, because a loop corresponds to a call to ThreadPool's
   * UNSAFE_emerg_yank routine, which can leave data in really bad shape.
   * Classes derived from Server that add new resources (e.g. a cache)
   * should use these callbacks to tear down and rebuild those resources. */
  Callback *onstartup, *onshutdown;

  LockFreeQueue<Work *> *q; // Jobs waiting to be worked on.
  int sigdl_int, sigdl_ext;
  FindWork *fwork;
  bool _doserve;
  sigset_t haltsigs;
  static Server *theserver;

protected:
  Scheduler *sch; // Not a great idea; accessor instead?
};

#endif // SERVER_HPP
