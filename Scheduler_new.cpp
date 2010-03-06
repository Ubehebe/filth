#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Scheduler_new.h"
#include "ServerErrs.h"

using namespace std;

Scheduler::Scheduler(LockedQueue<Work *> &q, mkWork &makework,
		     int pollsz, int maxevents)
  : q(q), makework(makework), maxevents(maxevents)
{

  // Set up the signal file descriptor.
  if (sigemptyset(&tohandle)==-1) {
    perror("sigemptyset");
    abort();
  }

  // Set up the polling file descriptor.
  if ((pollfd = epoll_create(pollsz))==-1) {
    perror("epoll_create");
    abort();
  }

  // The default behavior of SIGINTs should be halting.
  push_sighandler(SIGINT, Scheduler::halt);
}

void Scheduler::schedule(Work *w, bool oneshot)
{
  struct epoll_event e;
  memset((void *) &e, 0, sizeof(e));
  e.data.fd = w->fd;
  e.events = (oneshot) ? EPOLLONESHOT : 0;
  switch (w->m) {
  case Work::read:
    e.events |= EPOLLIN;
    break;
  case Work::write:
    e.events |= EPOLLOUT;
    break;
  }
  if (epoll_ctl(pollfd, EPOLL_CTL_ADD, w->fd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_ADD)", errno);
  delete w;
}

void Scheduler::reschedule(Work *w, bool oneshot)
{
  struct epoll_event e;
  memset((void *) &e, 0, sizeof(e));
  e.data.fd = w->fd;
  e.events = (oneshot) ? EPOLLONESHOT : 0;
  switch (w->m) {
  case Work::read:
    e.events |= EPOLLIN;
    break;
  case Work::write:
    e.events |= EPOLLOUT;
    break;
  }
  if (epoll_ctl(pollfd, EPOLL_CTL_MOD, w->fd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_MOD)", errno);
  delete w;
}

void Scheduler::poll()
{
  /* We don't do this in the constructor because the user might call
   * push_sighandler (several times) between the constructor and
   * starting the poll loop. */
  if ((sigfd = signalfd(-1, &tohandle, SFD_NONBLOCK))==-1) {
    perror("signalfd");
    abort();
  }

  // Don't try putting these in the constructor =)
  schedule(makework(listenfd, Work::read), false);
  schedule(makework(sigfd, Work::read), false);

  struct epoll_event fds[maxevents];
  int fd, nchanged, connfd, flags, so_err;
  socklen_t so_errlen = static_cast<socklen_t>(sizeof(so_err));

  dowork = true;

  while (dowork) {
    if ((nchanged = epoll_wait(pollfd, fds, maxevents, -1))<0) {
      throw ResourceErr("epoll_wait", errno);
    }
    /* This is sequential, but should not be a bottleneck because no
     * blocking is involved. I suppose we could put the fd's of interest
     * into a queue and have workers inspect them...probably overkill. */

    /* Stream sockets are full-duplex, so it is reasonable to expect
     * that they could be simultaneously readable and writable. But
     * schedule() and reschedule(), which are the only interfaces to the
     * scheduler's epoll data structures, set at most one of EPOLLIN and
     * EPOLLOUT. Therefore (I hope!) we should never see both
     * EPOLLIN and EPOLLOUT for the same file descriptor. In the traditional
     * poll/select API, sockets with errors are both readable and writable;
     * but I am hoping that in the epoll API, errors will show up as
     * EPOLLERR.
     *
     * There might be a subtle race condition here. If two workers are ever
     * servicing the same client, one might schedule a read and the other
     * might schedule a write. The semantics of epoll_ctl say that whoever
     * schedules last will win, therefore we might lose a scheduled read or
     * write. For this reason, I hope I can maintain the invariant:
     *
     * AT ANY TIME, THERE IS AT MOST ONE WORKER TALKING TO A GIVEN
     * CLIENT.
     *
     * Note that we could still have multiple workers working on behalf of a
     * single client, as long as only one was using the connected socket.
     * (For example, once a worker has parsed enough of a request to
     * identify the resource, another worker might be enlisted to prepare
     * the resource; but the first worker should ultimately send it back to
     * the client. */
    for (int i=0; i<nchanged; ++i) {
      fd = fds[i].data.fd;
      // epoll_wait always collects these. Do we know what to do with them?
      if ((fds[i].events & EPOLLERR) || (fds[i].events & EPOLLHUP))
	cerr << "Scheduler::poll: hangup or error, descriptor " << fd << endl;
      else if (fd == sigfd && (fds[i].events & EPOLLIN))
	handle_sigs();
      else if (fd == listenfd && (fds[i].events & EPOLLIN))
	handle_listen();
      else if (fds[i].events & EPOLLIN)
	q.enq(makework(fd, Work::read));
      else if (fds[i].events & EPOLLOUT)
	q.enq(makework(fd, Work::write));
      
      /* A handler might have set dowork to false; this will cause the
       * scheduler to fall through the poll loop right away. */
      if (!dowork) break;
    }
  }
  cerr << "scheduler retiring\n";
  q.enq(NULL); // Poison pill for the workers
}

void Scheduler::handle_listen()
{
  int acceptfd;
  if ((acceptfd = accept4(listenfd, NULL, NULL, SOCK_NONBLOCK))==-1) {
    // See "Unix Network Programming", 2nd ed., sec. 16.6 for rationale.
    if (errno == EWOULDBLOCK
	|| errno == ECONNABORTED
	|| errno == EPROTO
	|| errno == EINTR)
      return;
    else throw SocketErr("accept4", errno);
  }
  schedule(makework(acceptfd, Work::read), true);
}

void Scheduler::handle_sigs()
{
  struct signalfd_siginfo siginfo[sighandlers.size()];
  map<int, void (*)(Scheduler *)>::iterator iter;
  ssize_t nread = read(sigfd, (void *)&siginfo, sizeof(siginfo));
  
  // There might be more than one pending signal.
  int i=0;
  while (nread > 0) {
    if ((iter = sighandlers.find(siginfo[i].ssi_signo)) != sighandlers.end()) {
      ((*iter).second)(this);
      cerr << "got signal " << siginfo[i].ssi_signo << endl;
    } else {
      cerr << "Scheduler::handle_sigs: warning:\n"
	   << "got signal " << siginfo[i].ssi_signo
	   << " but have no handler for it, ignoring\n";
    }
    /* The signal handler could have set dowork to false, so we should
     * return immediately. */
    if (!dowork) return;
    nread -= sizeof(struct signalfd_siginfo);
    i++;
  }
}

void Scheduler::push_sighandler(int signo, void (*handler)(Scheduler *))
{
  if (sighandlers.find(signo) != sighandlers.end()) {
    cerr << "Scheduler::push_sighandler: warning:\n"
	 << "redefining signal handler for signal " << signo << endl;
  }
  else if (sigaddset(&tohandle, signo)==-1) {
    perror("sigaddset");
    abort();
  }    
  sighandlers[signo] = handler; 
}

void Scheduler::halt(Scheduler *s)
{
  cerr << "scheduler halting\n";
  s->dowork = false;
}

void Scheduler::flush(Scheduler *s)
{

}

void Scheduler::handle_sock_err(int fd)
{

}

void Scheduler::set_listenfd(int _listenfd)
{
  listenfd = _listenfd;
}
