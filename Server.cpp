#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "ServerErrs.h"
#include "Server.h"

// No exception handling in here; every error is fatal.
Server::Server(int domain, char const *bindto, void (*handle_SIGINT)(int), void (*at_exit)(),
	       Parsing &pars, int nworkers, time_t to_sec, int listenq)
  : domain(domain), listenq(listenq), nworkers(nworkers), to_sec(to_sec), workers(nworkers),
    pars(pars), sch(q)
{
  // Set SIGINT handler.
  struct sigaction act;
  memset((void *) &act, 0, sizeof(act));
  act.sa_handler = handle_SIGINT;
  /* If the user is really impatient, the second SIGINT
   * should just terminate the program. */
  act.sa_flags = SA_RESETHAND;
  if (sigaction(SIGINT, &act, NULL)==-1) {
    perror("sigaction");
    _exit(1);
  }

  // Set atexit handler.
  if ((errno = atexit(at_exit))!=0) {
    perror("atexit");
    _exit(1);
  }

  if ((listenfd = socket(domain, SOCK_STREAM, 0))==-1) {
    perror("socket");
    _exit(1);
  }

  // TCP socket. TODO: support IPv6.
  if (domain == AF_INET) {
    struct sockaddr_in sa;
    struct addrinfo *res;

    if ((errno = getaddrinfo(NULL, bindto, NULL, &res)) != 0) {
      perror("getaddrinfo");
      _exit(1);
    }

    memcpy((void *)&sa, (struct sockaddr_in *) res->ai_addr, sizeof(sa));
    freeaddrinfo(res);

    if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa))==-1) {
      perror("bind");
      _exit(1);
    }
  }
  // Unix domain socket (for CGI)
  else if (domain == AF_LOCAL) {
    
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
      _exit(1);
    }
  }
  else {
    errno = EINVAL; // domain isn't AF_INET or AF_LOCAL
    perror("unknown domain");
    _exit(1);
  }


  // Make listening socket nonblocking.
  int flags;
  if ((flags = fcntl(listenfd, F_GETFL))==-1) {
    perror("fcntl (F_GETFL)");
    _exit(1);
  }
  if (fcntl(listenfd, F_SETFL, flags|O_NONBLOCK)==-1) {
    perror("fcntl (F_SETFL)");
    _exit(1);
  }
  if (listen(listenfd, listenq)==-1) {
    perror("listen");
    _exit(1);
  }

  /* Set up workers. It's cumbersome to use generate_n because
   * we can't pass a static function. */
  for (std::vector<Worker *>::iterator it = workers.begin();
       it != workers.end(); ++it)
    *it = new Worker(&q, &sch, &state, &pars, to_sec);
}

Server::~Server()
{
  // If a local socket, attempt to unlink from the filesystem.
  if (domain == AF_LOCAL) {
    struct sockaddr_un sa;
    socklen_t salen = static_cast<socklen_t>(sizeof(sa));
    // We currently don't print an error if getsockname fails. Should we?
    if (getsockname(listenfd, (struct sockaddr *)&sa, &salen)==0) {
      if (unlink(sa.sun_path)==-1) {
	perror("unlink");
	std::cerr << "warning: failed to unlink " << sa.sun_path
		  << "You may want to remove it manually.\n";
      }
      else {
	std::cerr << sa.sun_path << " unlinked\n";
      }
    }
  }

  std::cerr << "retiring " << nworkers << " workers:\n";

  for_each(workers.begin(), workers.end(), Server::deleteWorker);
  for_each(state.begin(), state.end(), Server::deleteState);
     
  /* This will cause Server::operator() to unblock, so we want
   * to do this last. */
  sch.shutdown();
}

void Server::deleteWorker(Worker *w)
{
  // This will block until the worker thread is done.
  w->shutdown();
  delete w;
}

void Server::deleteState(std::pair<int,
			 std::pair<std::stringstream *, Parsing *> > p)
{
  delete p.second.first;
  delete p.second.second;
}
