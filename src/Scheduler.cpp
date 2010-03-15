#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "Scheduler.hpp"
#include "ServerErrs.hpp"
#include "sigmasks.hpp"

namespace handler_sch {
  Scheduler *s = NULL;
}

using namespace std;

Scheduler::Scheduler(LockedQueue<Work *> &q, Work &wmake,
		     int pollsz, int maxevents)
  : q(q), wmake(wmake), maxevents(maxevents)
{
  /* I didn't design the scheduler class to have more than one instantiation
   * at a time, but it could support that, with the caveat that signal handlers
   * only know about one instance. If we really need support for this,
   * I guess it wouldn't be too hard to make handler_sch::s a vector/array. */
  if (handler_sch::s == NULL)
    handler_sch::s = this;

  // Set up the signal file descriptor.
  if (sigemptyset(&tohandle)==-1) {
    _LOG_FATAL("sigemptyset: %m");
    exit(1);
  }
  
  /* Test whether we have the signalfd system call. */
  int dummyfd;
  if ((dummyfd = signalfd(-1, &tohandle, 0))==-1)
    use_signalfd = false;
  else {
    use_signalfd = true;
    close(dummyfd);
  }
  _LOG_DEBUG("%s using signalfd", (use_signalfd) ? "" : " not");

  // Set up the polling file descriptor.
  if ((pollfd = epoll_create(pollsz))==-1) {
    _LOG_FATAL("epoll_create: %m");
    exit(1);
  }
  _LOG_DEBUG("polling fd is %d", pollfd);

  // The default behavior of SIGINTs should be halting.
  push_sighandler(SIGINT, Scheduler::halt);
}

void Scheduler::register_special_fd(int fd, void (*cb)(uint32_t),
				    Work::mode m, bool oneshot)
{
  if (fd < 0) {
    _LOG_INFO("invalid file descriptor %d, ignoring", fd);
    return;
  } else if (fd == listenfd) {
    _LOG_INFO("%d identical to listening fd, ignoring", fd);
    return;
  } else if (fd == pollfd) {
    _LOG_INFO("%d identical to polling fd, ignoring", fd);
    return;
  }
  // We don't compare against sigfd because that isn't set until poll().

  unordered_map<int, void (*)(uint32_t)>::iterator it;
  if ((it = special_fd_handlers.find(fd)) != special_fd_handlers.end()) {
    _LOG_INFO("redefining handler for %d", fd);
    it->second = cb;
  }
  else { 
    special_fd_handlers[fd] = cb;
  }
  schedule(wmake.getwork(fd, m), oneshot);
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
}

void Scheduler::poll()
{
  if (use_signalfd) {
    sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);
    if ((sigfd = signalfd(-1, &tohandle, SFD_NONBLOCK))==-1) {
      _LOG_FATAL("signalfd: %m");
      exit(1);
    }
    _LOG_DEBUG("signal fd is %d", sigfd);
    schedule(wmake.getwork(sigfd, Work::read), false);
  }

  else {
    sigmasks::sigmask_caller(sigmasks::BLOCK_NONE);
  }

  schedule(wmake.getwork(listenfd, Work::read), false);

  struct epoll_event fds[maxevents];
  int fd, nchanged, connfd, flags, so_err;
  socklen_t so_errlen = static_cast<socklen_t>(sizeof(so_err));

  dowork = true;

  while (dowork) {
    /* epoll_wait is a slow system call, meaning that it can be interrupted
     * by a signal. With the signalfd mechanism this cannot happen, since
     * signals are delivered to sigfd. But it can happen when the fallback
     * mechanism of old-fashioned signal handlers. In this case
     * we just continue; a signal attached to the "halt" handler will
     * set dowork to false, so we'll just fall through the loop.
     * TODO: what about other signal behavior, e.g. flush? */
    if ((nchanged = epoll_wait(pollfd, fds, maxevents, -1))<0
	&& errno != EINTR)
	throw ResourceErr("epoll_wait", errno);

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
    unordered_map<int, void (*)(uint32_t)>::iterator it;
    for (int i=0; i<nchanged; ++i) {
      fd = fds[i].data.fd;
      // If we have a special handler for this fd, use that.
      if ((it = special_fd_handlers.find(fd)) != special_fd_handlers.end())
	it->second(fds[i].events);
      // epoll_wait always collects these. Do we know what to do with them?
      else if ((fds[i].events & EPOLLERR) || (fds[i].events & EPOLLHUP))
	_LOG_INFO("hangup or error on %d", fd);
      else if (use_signalfd && fd == sigfd && (fds[i].events & EPOLLIN))
	handle_sigs();
      else if (fd == listenfd && (fds[i].events & EPOLLIN))
	handle_accept();
      // HMM. Do we need to check or change the work object's read/write mode?
      else if (fds[i].events & EPOLLIN)
	q.enq(wmake.getwork(fd, Work::read));
      else if (fds[i].events & EPOLLOUT)
	q.enq(wmake.getwork(fd, Work::write));
      /* A handler might have set dowork to false; this will cause the
       * scheduler to fall through the poll loop right away. */
      if (!dowork) break;
    }
  }
  _LOG_INFO("scheduler retiring");
  q.enq(NULL); // Poison pill for the workers
}

void Scheduler::handle_accept()
{
  int acceptfd;
  if ((acceptfd = accept(listenfd, NULL, NULL))==-1) {
    // See "Unix Network Programming", 2nd ed., sec. 16.6 for rationale.
    if (errno == EWOULDBLOCK
	|| errno == ECONNABORTED
	|| errno == EPROTO
	|| errno == EINTR) {
      _LOG_DEBUG("accept: %m");
      return;
    }
    else throw SocketErr("accept", errno);
  }
  int flags;
  if ((flags = fcntl(acceptfd, F_GETFL))==-1)
    throw SocketErr("fcntl (F_GETFL)", errno);
  if (fcntl(acceptfd, F_SETFL, flags | O_NONBLOCK)==-1)
    throw SocketErr("fcntl (F_SETFL)", errno);
  schedule(wmake.getwork(acceptfd, Work::read), true);
}

void Scheduler::handle_sigs()
{
  struct signalfd_siginfo siginfo[sighandlers.size()];
  unordered_map<int, void (*)(int)>::iterator iter;
  ssize_t nread = read(sigfd, (void *)&siginfo, sizeof(siginfo));
  
  // There might be more than one pending signal.
  int i=0;
  while (nread > 0) {
    if ((iter = sighandlers.find(siginfo[i].ssi_signo)) != sighandlers.end()) {
      ((*iter).second)(0);
      _LOG_INFO("got signal %s", strsignal(siginfo[i].ssi_signo));
    } else {
      _LOG_INFO("got signal %s but have no handler for it, ignoring",
		strsignal(siginfo[i].ssi_signo));
    }
    /* The signal handler could have set dowork to false, so we should
     * return immediately. */
    if (!dowork) return;
    nread -= sizeof(struct signalfd_siginfo);
    i++;
  }
}

void Scheduler::push_sighandler(int signo, void (*handler)(int))
{
  if (use_signalfd) {
    if (sighandlers.find(signo) != sighandlers.end()) {
      _LOG_INFO(" redefining signal handler for signal %d", signo);
    }
    else if (sigaddset(&tohandle, signo)==-1) {
      _LOG_FATAL("sigaddset: %m");
      exit(1);
    }
    sighandlers[signo] = handler; 
  }

  else {
    struct sigaction act;
    memset((void *)&act, 0, sizeof(act));
    if (sigemptyset(&act.sa_mask)!=0) {
      _LOG_FATAL("sigemptyset: %m");
      exit(1);
    }
    act.sa_handler = handler;
    if (sigaction(signo, &act, NULL)==-1) {
      _LOG_FATAL("sigaction: %m");
      exit(1);
    }    
  }
}

void Scheduler::halt(int ignore)
{
  _LOG_INFO("scheduler halting");
  handler_sch::s->dowork = false;
}

void Scheduler::flush(int ignore)
{
  // TODO
}

void Scheduler::handle_sock_err(int fd)
{
  // What's this for?
}

void Scheduler::set_listenfd(int _listenfd)
{
  listenfd = _listenfd;
}
