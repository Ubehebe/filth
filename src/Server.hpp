#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <list>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <unordered_map>

#include "Factory.hpp"
#include "FindWork_prealloc.hpp"
#include "LockFreeQueue.hpp"
#include "Locks.hpp"
#include "logging.h"
#include "Scheduler.hpp"
#include "sigmasks.hpp"
#include "ThreadPool.hpp"
#include "Work.hpp"
#include "Worker.hpp"

template<class _Work, class _Worker> class Server
{
public:
  /* For network sockets, bindto should be a string of a port number,
   * like "80". For local sockets, bindto should be a filesystem path. */
  Server(
	 int domain,
	 char const *mount,
	 char const *bindto,
	 int nworkers=10,
	 int listenq=100,
	 size_t preallocMB=10,
	 char const *ifnam=NULL,
	 sigset_t *haltsigs=NULL,
	 int sigdl_int=-1,
	 int sigdl_ext=-1,
	 int tcp_keepalive_intvl=-1,
	 int tcp_keepalive_probes=-1,
	 int tcp_keepalive_time=-1);
  /* These are hooks into beginning and end of the server's serve() loop. 
   * The idea is that the server should tear down and rebuild all its resources
   * for each loop, because a loop corresponds to a call to ThreadPool's
   * UNSAFE_emerg_yank routine, which can leave data in really bad shape.
   * Classes derived from Server that add new resources (e.g. a cache)
   * should use these callbacks to tear down and rebuild those resources. */
  virtual void onstartup() {}
  virtual void onshutdown() {}
  virtual ~Server();
  void serve();
  void doserve(bool doserve) { _doserve = doserve; }
  static void halt(int ignore=-1);
  static void UNSAFE_emerg_yank_wrapper(int ignore=-1);

private:
  Server &operator=(Server const&);
  Server(Server const&);

  // These two are pointers because they can get rebuilt within the main loop.
  FindWork_prealloc<_Work> *findwork;
  /* Jobs waiting to be worked on.
   * N.B.: "Work", not "_Work". The reason is that although there is usually
   * one kind of work that characterizes a server, the server may occasionally
   * undertake other kinds of work, for example when it acts as a client to
   * another server on behalf of the "real" client. Any old piece of work
   * can go in the job queue, as long as workers know how to deal with it. */
  LockFreeQueue<Work *> *jobq;

  // This is not a pointer because there is no need to rebuild it.
  Factory<_Worker> wfact;

  // Internal setup functions for supported domains.
  void setup_AF_INET();
  void setup_AF_INET6();
  void setup_AF_LOCAL();
  void socket_bind_listen();

  int domain, listenfd, listenq, nworkers;

  int tcp_keepalive_intvl, tcp_keepalive_probes, tcp_keepalive_time;

  char const *bindto, *ifnam;
  size_t const preallocMB;

  /* A server bound to a socket in the filesystem needs to remember both
   * where it is bound (sockdir) and the subtree of the filesystem it considers
   * root (mntdir). This is because if the server does a soft reboot,
   * it will have to chdir to sockdir, bind the socket there, then chdir to
   * mntdir to service requests from there. */
  char *sockdir; // not const because it uses get_current_dir_name
  char const *mntdir;

  int sigdl_int, sigdl_ext;
  bool _doserve;
  sigset_t haltsigs;
  static Server *theserver;
  Scheduler *sch; // Not a great idea; accessor instead?
};

template<class _Work, class _Worker>
Server<_Work, _Worker> *Server<_Work, _Worker>::theserver;

template<class _Work, class _Worker>
Server<_Work, _Worker>::Server(
	       int domain,
	       char const *mount,
	       char const *bindto,
	       int nworkers,
	       int listenq,
	       size_t preallocMB,
	       char const *ifnam,
	       sigset_t *_haltsigs,
	       int sigdl_int,
	       int sigdl_ext,
	       int tcp_keepalive_intvl,
	       int tcp_keepalive_probes,
	       int tcp_keepalive_time)
  : domain(domain), bindto(bindto), preallocMB(preallocMB),
    nworkers(nworkers), listenq(listenq), sigdl_int(sigdl_int),
    sigdl_ext(sigdl_ext), ifnam(ifnam),
    tcp_keepalive_intvl(tcp_keepalive_intvl),
    tcp_keepalive_probes(tcp_keepalive_probes),
    tcp_keepalive_time(tcp_keepalive_time),
    _doserve(true)
{
  // For signal handlers that have to be static.
  theserver = this;

  if (_haltsigs != NULL) {
    memcpy((void *)&haltsigs, (void *)_haltsigs, sizeof(sigset_t));
  } else if (sigemptyset(&haltsigs)==-1) {
    _LOG_FATAL("sigemptyset: %m");
    exit(1);
  } else if (sigaddset(&haltsigs, SIGINT)==-1) {
    _LOG_FATAL("sigaddset: %m");
    exit(1);
  } else if (sigaddset(&haltsigs, SIGTERM)==-1) {
    _LOG_FATAL("sigaddset: %m");
    exit(1);
  }

  /* For non-local sockets, go to the mount point. (For local sockets, we need
   * to wait until we bind the socket to the filesystem.) */
  if (domain == AF_LOCAL) {
    // This allocates memory that needs to be freed in the destructor.
    sockdir = get_current_dir_name();
    // This doesn't.
    mntdir = mount;
    /* If the user gave a long name that got truncated to a name that already
     * existed in the filesystem, that file could accidentally be unlinked. */
    struct sockaddr_un sa;
    if (strlen(bindto) > sizeof(sa.sun_path)-1) {
      _LOG_FATAL("domain socket name %s is too long "
		 "and don't want to silently truncate (max len %d)",
		 bindto, sizeof(sa.sun_path)-1);
      exit(1);
    }
  }
  else if (chdir(mount)==-1) {
    _LOG_FATAL("chdir: %m");
    exit(1);
  }
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::halt(int ignore)
{ 
  // Causes the server to end after the next iteration of serve()
  theserver->doserve(false);
  // Causes the current iteration of serve() to end
  theserver->sch->halt();
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::UNSAFE_emerg_yank_wrapper(int ignore)
{
  // I forgot what this does :)
  ThreadPool<_Worker>::UNSAFE_emerg_yank();
  // Causes the current iteration of serve() to end
  theserver->sch->halt();
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::socket_bind_listen()
{
  if ((listenfd = socket(domain, SOCK_STREAM, 0))==-1) {
    _LOG_FATAL("socket: %m");
    exit(1);
  }

  switch (domain) {
  case AF_INET: setup_AF_INET(); break;
  case AF_INET6: setup_AF_INET6(); break;
  case AF_LOCAL: setup_AF_LOCAL(); break;
  default: 
    _LOG_FATAL("unsupported domain %d", domain);
    exit(1);
  }

  if (domain != AF_LOCAL
      && (tcp_keepalive_intvl != -1
	  || tcp_keepalive_probes != -1
	  || tcp_keepalive_time != -1)) {
    int turnon = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (void *) &turnon,
		   (socklen_t) sizeof(turnon))==-1) {
      _LOG_FATAL("setsockopt (SO_KEEPALIVE): %m");
    exit(1);
    }
    if (tcp_keepalive_intvl != -1
	&& setsockopt(listenfd, SOL_TCP, TCP_KEEPINTVL,
		      (void *) &tcp_keepalive_intvl,
		      (socklen_t) sizeof(tcp_keepalive_intvl))==-1) {
      _LOG_FATAL("setsockopt (TCP_KEEPINTVL): %m");
      exit(1);
    }
    
    if (tcp_keepalive_probes != -1
	&& setsockopt(listenfd, SOL_TCP, TCP_KEEPCNT,
		      (void *) &tcp_keepalive_probes,
		      (socklen_t) sizeof(tcp_keepalive_probes))==-1) {
      _LOG_FATAL("setsockopt (TCP_KEEPCNT): %m");
      exit(1);
    }
    
    if (tcp_keepalive_time != -1
	&& setsockopt(listenfd, SOL_TCP, TCP_KEEPIDLE,
		      (void *) &tcp_keepalive_time,
		      (socklen_t) sizeof(tcp_keepalive_time))==-1) {
      _LOG_FATAL("setsockopt (TCP_KEEPIDLE): %m");
      exit(1);
    }
  }

  // Everything below should be domain-independent.

  // Make listening socket nonblocking.
  int flags;
  if ((flags = fcntl(listenfd, F_GETFL))==-1) {
    _LOG_FATAL("fcntl (F_GETFL): %m");
    exit(1);
  }
  if (fcntl(listenfd, F_SETFL, flags|O_NONBLOCK)==-1) {
    _LOG_FATAL("fcntl (F_SETFL): %m");
    exit(1);
  }
  
  if (listen(listenfd, listenq)==-1) {
    _LOG_FATAL("listen: %m");
    exit(1);
    }
  _LOG_DEBUG("listen fd is %d", listenfd);

}

template<class _Work, class _Worker>
Server<_Work, _Worker>::~Server()
{
  if (domain == AF_LOCAL) {
    if (chdir(sockdir)==-1) {
      _LOG_INFO("chdir %s: %m, ignoring (so can't unlink socket)", sockdir);
    }
    else {
      _LOG_DEBUG("unlink %s", bindto);
    if (unlink(bindto)==-1)
      _LOG_INFO("unlink %s: %m, ignoring", bindto);
    }
    free(sockdir); // because it was a strdup, basically
  }
}

// The main loop.
template<class _Work, class _Worker>
void Server<_Work, _Worker>::serve()
{
  sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);

  while (_doserve) {
    socket_bind_listen();
    _Worker::jobq = jobq = new LockFreeQueue<Work *>();
    sch = new Scheduler(*jobq, listenfd);
    findwork = new FindWork_prealloc<_Work>(preallocMB * (1<<20));
    sch->setfwork(findwork);
    _Work::setsch(sch);
    onstartup();
    
    // No signal has value 0.
    for (uint8_t sig=1; sig<NSIG; ++sig)
      if (sigismember(&haltsigs, sig))
	sch->push_sighandler(sig, halt);

    if (sigdl_ext != -1)
      sch->push_sighandler(sigdl_ext, UNSAFE_emerg_yank_wrapper);

    { // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
      Thread<Scheduler> schedth(sch, &Scheduler::poll);

      ThreadPool<_Worker> wths(wfact, &_Worker::work, nworkers, sigdl_int);
      schedth.start();
      wths.start();
      /* The Thread and ThreadPool destructors wait for their threads 
       * to go out of scope. */
    } // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

    delete findwork;
    delete sch;
    delete jobq;
    onshutdown();
  }
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::setup_AF_INET()
{
  struct ifaddrs *ifap;

  if (getifaddrs(&ifap)==-1) {
    _LOG_FATAL("getifaddrs: %m");
    exit(1);
  }

  struct ifaddrs *tmp;
  for (tmp = ifap; tmp != NULL; tmp = tmp->ifa_next) {
    if (strncmp(tmp->ifa_name, ifnam, strlen(tmp->ifa_name))==0
	&& tmp->ifa_addr->sa_family == AF_INET)
      break;
  }
  if (tmp == NULL) {
    printf("interface %s not found for family AF_INET\n", ifnam);
    exit(1);
  }

  struct sockaddr_in sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(atoi(bindto));
  sa.sin_addr.s_addr
    = ((struct sockaddr_in *) (tmp->ifa_addr))->sin_addr.s_addr;

  freeifaddrs(ifap);

  // This should catch the case when portno is bad.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    _LOG_FATAL("bind: %m");
    exit(1);
  }
  socklen_t salen = sizeof(sa);
  if (getsockname(listenfd, (struct sockaddr *) &sa, &salen)==-1) {
    _LOG_FATAL("getsockname: %m");
    exit(1);
  }

  char ipnam[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, (void *) &sa.sin_addr, ipnam, INET_ADDRSTRLEN)
      ==NULL) {
    _LOG_FATAL("inet_ntop: %m");
    exit(1);
  }
  _LOG_INFO("listening on %s:%d", ipnam, ntohs(sa.sin_port));
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::setup_AF_INET6()
{
  struct ifaddrs *ifap;

  if (getifaddrs(&ifap)==-1) {
    _LOG_FATAL("getifaddrs: %m");
    exit(1);
  }

  struct ifaddrs *tmp;
  for (tmp = ifap; tmp != NULL; tmp = tmp->ifa_next) {
    if (strncmp(tmp->ifa_name, ifnam, strlen(tmp->ifa_name))==0
	&& tmp->ifa_addr->sa_family == AF_INET6)
      break;
  }
  if (tmp == NULL) {
    _LOG_FATAL("interface %s not found for family AF_INET6", ifnam);
    exit(1);
  }

  struct sockaddr_in6 sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sin6_family = AF_INET6;
  sa.sin6_port = htons(atoi(bindto));
  memcpy((void *) &sa.sin6_addr,
	 (void *) (&((struct sockaddr_in6*) (tmp->ifa_addr))->sin6_addr),
	 sizeof(struct in6_addr));

  freeifaddrs(ifap);
  
  // This should catch the case when portno is bad.
  /* TODO: why does bind succeed for ipv6 on loopback interface
   * but not e.g. eth0? */
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    _LOG_FATAL("bind: %m");
    exit(1);
  }
  socklen_t salen = sizeof(sa);
  if (getsockname(listenfd, (struct sockaddr *) &sa, &salen)==-1) {
    _LOG_FATAL("getsockname: %m");
    exit(1);
  }

  char ipnam[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, (void *) &sa.sin6_addr, ipnam, INET6_ADDRSTRLEN)
      ==NULL) {
    _LOG_FATAL("inet_ntop: %m");
    exit(1);
  }
  _LOG_INFO("listening on %s:%d", ipnam, ntohs(sa.sin6_port));
}

template<class _Work, class _Worker>
void Server<_Work, _Worker>::setup_AF_LOCAL()
{
  if (chdir(sockdir)==-1) {
    _LOG_FATAL("chdir: %m");
    exit(1);
  }

  struct sockaddr_un sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sun_family = AF_LOCAL;
    
  // We have already checked (in the constructor) that bindto isn't truncated.
  strncpy(sa.sun_path, bindto, sizeof(sa.sun_path)-1);

  /* We might be doing a soft reboot of the server, and the previous socket
   * might still be bound in the filesystem, so get rid of it. This won't
   * accidentally unlink a file that happens to have the same name as
   * the desired socket; the server constructor checks for that. */
  unlink(sa.sun_path);

  // This will fail if the path exists, which is what we want.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    _LOG_FATAL("bind: %m");
    exit(1);
  }
  _LOG_INFO("listening on %s", sa.sun_path);

  if (chdir(mntdir)==-1) {
    _LOG_FATAL("chdir: %m");
    exit(1);
  }
}

#endif // SERVER_HPP
