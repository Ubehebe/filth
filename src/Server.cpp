#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <list>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "Factory.hpp"
#include "logging.h"
#include "Server.hpp"
#include "sigmasks.hpp"
#include "ThreadPool.hpp"

Server::Server(
	       int domain,
	       FindWork *fwork,
	       char const *mount,
	       char const *bindto,
	       int nworkers,
	       int listenq,
	       int sigdl_int,
	       int sigdl_ext,
	       char const *ifnam,
	       Callback *onstartup,
	       Callback *onshutdown)
  : domain(domain), fwork(fwork), bindto(bindto),
    nworkers(nworkers), listenq(listenq), sigdl_int(sigdl_int),
    sigdl_ext(sigdl_ext), ifnam(ifnam), onstartup(onstartup),
    onshutdown(onshutdown), doserve(true)
{
  /* A domain socket server needs to remember where it's bound in the
   * filesystem in order to unlink in the destructor. */
  if (domain == AF_LOCAL)
    sockdir = get_current_dir_name();

  // Go to the mount point.
  if (chdir(mount)==-1) {
    _LOG_FATAL("chdir: %m");
    exit(1);
  }
}

void Server::socket_bind_listen()
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

Server::~Server()
{
  /* Typically, a FooServer will inherit from Server and also contain a
   * FooFindWork object that keeps track of (and maybe allocates/deallocates)
   * FooWork objects. Thus, in the FooServer destructor, the FooFindWork
   * destructor is called _before_ the Server destructor, i.e. before now.
   * Thus, we do not need to worry about winding down still-active
   * connections; they have already been dealt with, even the listening
   * socket. */

  if (domain == AF_LOCAL) {
    if (chdir(sockdir)==-1) {
      _LOG_INFO("chdir %s: %m, ignoring (so can't unlink socket)", sockdir);
    }
    else {
      _LOG_DEBUG("unlink %s", bindto);
    if (unlink(bindto)==-1)
      _LOG_INFO("unlink %s: %m, ignoring", bindto);
    }
    free(sockdir);
  }
}

// The main loop.
void Server::serve()
{
  sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);

  while (doserve) {
    socket_bind_listen();
    q = new DoubleLockedQueue<Work *>();
    sch = new Scheduler(*q, listenfd);

    /* This callback should create a FindWork object and let the scheduler 
     * know about it. */
    if (onstartup != NULL)
      (*onstartup)();

    {
      
      Thread<Scheduler> schedth(sch, &Scheduler::poll);
      Factory<Worker> wfact(q);
      ThreadPool<Worker> wths(wfact, &Worker::work, nworkers, sigdl_int);
      schedth.start();
      wths.start();
      /* The Thread and ThreadPool destructors wait for their threads 
       * to go out of scope.
       *
       * BUG
       * The emergency yank still does not work correctly. I get a segfault at
       * what looks to around the Thread destructor for the scheduler.
       * Suggestively, when I run it under valgrind, there is no segfault and
       * not even any warning. What could be the difference? */
    }
    _LOG_DEBUG("Thread and ThreadPool went out of scope");

    if (onshutdown != NULL)
      (*onshutdown)();
    delete sch;
    delete q;
    _LOG_DEBUG("looping");
  }
}

void Server::setup_AF_INET()
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

void Server::setup_AF_INET6()
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

void Server::setup_AF_LOCAL()
{
  struct sockaddr_un sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sun_family = AF_LOCAL;
    
  strncpy(sa.sun_path, bindto, sizeof(sa.sun_path)-1);

  if (strlen(bindto) > sizeof(sa.sun_path)-1) {
    _LOG_INFO("domain socket name %s is too long, truncating to %s",
	      bindto, sa.sun_path);
  }

  // This will fail if the path exists, which is what we want.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    _LOG_FATAL("bind: %m");
    exit(1);
  }
  _LOG_INFO("listening on %s", sa.sun_path);
}
