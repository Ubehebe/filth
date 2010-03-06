#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <list>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "Server_new.hpp"

Server::Server(int domain, mkWork &makework, char const *bindto,
	       char const *ifnam, int nworkers, int listenq)
  : domain(domain), listenq(listenq), nworkers(nworkers), q(), sch(q, makework)
{
  if ((listenfd = socket(domain, SOCK_STREAM, 0))==-1) {
    perror("socket");
    abort();
  }

  // So the workers know where to get work.
  Worker::q = &q;

  switch (domain) {
  case AF_INET: setup_AF_INET(bindto, ifnam); break;
  case AF_INET6: setup_AF_INET6(bindto, ifnam); break;
  case AF_LOCAL: setup_AF_LOCAL(bindto); break;
  default: std::cerr << "Server::Server: unsupported domain "
		     << domain << ", aborting\n";
    abort();
  }

  // Everything below should be domain-independent.

  // Make listening socket nonblocking.
  int flags;
  if ((flags = fcntl(listenfd, F_GETFL))==-1) {
    perror("fcntl (F_GETFL)");
    abort();
  }
  if (fcntl(listenfd, F_SETFL, flags|O_NONBLOCK)==-1) {
    perror("fcntl (F_SETFL)");
    abort();
  }
  if (listen(listenfd, listenq)==-1) {
    perror("listen");
    abort();
  }

  // listenfd was meaningless until we bound it.
  sch.set_listenfd(listenfd);
}

void Server::serve()
{

  // All signals go to the scheduler...
  block_all_signals();
  /* ...what I really mean is they go to the signal fd IN the scheduler.
   * This guy has to be named in order not to go out of scope immediately. */
  Thread<Scheduler> _blah(&sch, &Scheduler::poll, sigmasks::BLOCK_ALL);

  Thread<Worker>::set_default_sigmask(sigmasks::BLOCK_ALL);
  std::list<Thread<Worker> *> workers;
  /* The Thread destructor calls pthread_join, so this should block until
   * all the workers and the scheduler are done. */
  for (int i=0; i<nworkers; ++i)
    workers.push_back(new Thread<Worker>(&Worker::work));
  for (std::list<Thread<Worker> *>::iterator it = workers.begin();
       it != workers.end(); ++it)
    delete *it;
}

void Server::setup_AF_INET(char const *portno, char const *ifnam)
{
  struct ifaddrs *ifap;

  if (getifaddrs(&ifap)==-1) {
    perror("getifaddrs");
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
  sa.sin_port = htons(atoi(portno));
  sa.sin_addr.s_addr
    = ((struct sockaddr_in *) (tmp->ifa_addr))->sin_addr.s_addr;

  freeifaddrs(ifap);

  // This should catch the case when portno is bad.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    perror("bind");
    abort();
  }
  socklen_t salen = sizeof(sa);
  if (getsockname(listenfd, (struct sockaddr *) &sa, &salen)==-1) {
    perror("getsockname");
    abort();
  }

  char ipnam[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, (void *) &sa.sin_addr, ipnam, INET_ADDRSTRLEN)
      ==NULL) {
    perror("inet_ntop");
    abort();
  }
  std::cout << "listening on " << ipnam << ":"
	    << ntohs(sa.sin_port) << std::endl;
}

void Server::setup_AF_INET6(char const *portno, char const *ifnam)
{
  struct ifaddrs *ifap;

  if (getifaddrs(&ifap)==-1) {
    perror("getifaddrs");
    exit(1);
  }

  struct ifaddrs *tmp;
  for (tmp = ifap; tmp != NULL; tmp = tmp->ifa_next) {
    if (strncmp(tmp->ifa_name, ifnam, strlen(tmp->ifa_name))==0
	&& tmp->ifa_addr->sa_family == AF_INET6)
      break;
  }
  if (tmp == NULL) {
    printf("interface %s not found for family AF_INET6\n", ifnam);
    exit(1);
  }

  struct sockaddr_in6 sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sin6_family = AF_INET6;
  sa.sin6_port = htons(atoi(portno));
  memcpy(
	 (void *) &sa.sin6_addr,
	 (void *) (&((struct sockaddr_in6*) (tmp->ifa_addr))->sin6_addr),
	 16);

  freeifaddrs(ifap);

 char ipnam[INET6_ADDRSTRLEN];
  if (inet_ntop(AF_INET6, (void *) &sa.sin6_addr, ipnam, INET6_ADDRSTRLEN)
      ==NULL) {
    perror("inet_ntop");
    abort();
  }
  std::cout << "listening on " << ipnam << ":"
	    << ntohs(sa.sin6_port) << std::endl;

  // This should catch the case when portno is bad.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    perror("bind");
    abort();
  }
  socklen_t salen = sizeof(sa);
  if (getsockname(listenfd, (struct sockaddr *) &sa, &salen)==-1) {
    perror("getsockname");
    abort();
  }
 
}

void Server::setup_AF_LOCAL(char const *bindto)
{
  struct sockaddr_un sa;
  memset((void *)&sa, 0, sizeof(sa));
  sa.sun_family = AF_LOCAL;
    
  strncpy(sa.sun_path, bindto, sizeof(sa.sun_path)-1);

  if (strlen(bindto) > sizeof(sa.sun_path)-1) {
    std::cerr << "warning: domain socket name "
	      << bindto << "is too long, truncating to " << sa.sun_path;
  }

  // This will fail if the path exists, which is what we want.
  if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
    perror("bind");
    abort();
  }
  std::cout << "listening on " << sa.sun_path << std::endl;
}

void Server::block_all_signals()
{
  sigset_t sigs;
  if (sigfillset(&sigs)==-1) {
    perror("sigfillset");
    abort();
  }
  if ((errno = pthread_sigmask(SIG_SETMASK, &sigs, NULL)) != 0) {
    perror("pthread_sigmask");
    abort();
  }
}
