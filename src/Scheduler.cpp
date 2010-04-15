#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
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

using namespace std;

Scheduler *Scheduler::thescheduler = NULL;

Scheduler::_acceptcb::_acceptcb(Scheduler &sch, FindWork *fwork, int listenfd)
  : sch(sch), fwork(fwork), fd(listenfd)
{
#ifdef _COLLECT_STATS
  accepts = 0;
#endif
}

Scheduler::_acceptcb::~_acceptcb()
{
  _SHOW_STAT(accepts);
}

Scheduler::Scheduler(ConcurrentQueue<Work *> &q, int listenfd,
		     FindWork *fwork, int pollsz, int maxevents)
  : q(q), fwork(fwork), maxevents(maxevents), dowork(true),
    acceptcb(*this, fwork, listenfd),
    sigcb(*this, fwork, dowork, sighandlers)
{
  /* I didn't design the scheduler class to have more than one instantiation
   * at a time, but it could support that, with the caveat that signal handlers
   * only know about one instance. If we really need support for this,
   * I guess it wouldn't be too hard to make handler_sch::s a vector/array. */
  thescheduler = this;

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
  _LOG_DEBUG("%susing signalfd", (use_signalfd) ? "" : "not ");

  // Set up the polling file descriptor.
  if ((pollfd = epoll_create(pollsz))==-1) {
    _LOG_FATAL("epoll_create: %m");
    exit(1);
  }
  _LOG_DEBUG("polling fd is %d", pollfd);
}

Scheduler::~Scheduler()
{
  _LOG_DEBUG("close %d", pollfd);
  close(pollfd);
}

void Scheduler::registercb(int fd, Callback *cb, Work::mode m, bool oneshot)
{
  if (fd < 0) {
    _LOG_INFO("invalid file descriptor %d, ignoring", fd);
    return;
  } 

  fdcb_map::iterator it;
  if ((it = fdcbs.find(fd)) != fdcbs.end()) {
    _LOG_INFO("redefining handler for %d", fd);
    it->second = cb;
  }
  else { 
    fdcbs[fd] = cb;
  }
  schedule((*fwork)(fd, m), oneshot);
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
  e.events |= EPOLLRDHUP; // ???
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
  e.events |= EPOLLRDHUP; // ???
  if (epoll_ctl(pollfd, EPOLL_CTL_MOD, w->fd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_MOD)", errno);
}

void Scheduler::poll()
{
  if (use_signalfd) {
    sigmasks::sigmask_caller(sigmasks::BLOCK_ALL);
    if ((sigcb.fd = signalfd(-1, &tohandle, SFD_NONBLOCK))==-1) {
      _LOG_FATAL("signalfd: %m");
      exit(1);
    }
    _LOG_DEBUG("signal fd is %d", sigcb.fd);
    registercb(sigcb.fd, &sigcb, Work::read, false);
  }

  else {
    sigmasks::sigmask_caller(sigmasks::BLOCK_NONE);
  }

  registercb(acceptcb.fd, &acceptcb, Work::read, false);

  struct epoll_event fds[maxevents];
  int fd, nchanged, connfd, flags, so_err;
  socklen_t so_errlen = static_cast<socklen_t>(sizeof(so_err));

  while (dowork) {
    /* epoll_wait is a slow system call, meaning that it can be interrupted
     * by a signal. With the signalfd mechanism this cannot happen, since
     * signals are delivered to sigfd. But it can happen when the fallback
     * mechanism of old-fashioned signal handlers. In this case
     * we just continue; a signal attached to the "halt" handler will
     * set dowork to false, so we'll just fall through the loop. */
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
     * Here is the crucial invariant for reasoning about the scheduler:
     * Any connection that shows up in the epoll_wait is neither currently
     * being worked on by a worker, nor in the job queue waiting for a worker.
     * (Rationale: new connections are registered with epoll_wait by the
     * scheduler, shortly after the accept call; at this time nobody else can
     * know about them. Every schedule or reschedule call uses the EPOLLONESHOT
     * flag, which means that after the first time a connection shows up in the
     * epoll_wait, it is internally disabled until another explicit call to reschedule.
     * Reschedules occur when a worker is done working on a connection.)
     */
    fdcb_map::iterator it;
    for (int i=0; i<nchanged; ++i) {
      fd = fds[i].data.fd;
      /* If we have a special handler for this fd, use that.
       * Note that we don't pass any information to the callback,
       * in particular the events. Do we need to? */
      if ((it = fdcbs.find(fd)) != fdcbs.end())
	(*(it->second))();
      /* In case of error or hangup, we can delete the information associated
       * with this connection immediately. No worker can be working on it,
       * and it can't be in the job queue waiting for a worker; see invariant
       * above.
       *
       * Previously, I was setting the work object's deleteme to true,
       * then enqueuing it, thinking that the next worker would delete it.
       * However, I was getting double free/corruption errors, which I still
       * don't understand. */
      else if ((fds[i].events & EPOLLERR)
	       || (fds[i].events & EPOLLHUP)
	       || (fds[i].events & EPOLLRDHUP)) {
	_LOG_INFO("hangup or error on %d", fd);
	Work *tmp = (*fwork)(fd, Work::read);
	delete tmp;
      }
      else if (fds[i].events & EPOLLIN) {
	q.enq((*fwork)(fd, Work::read));
      }
      else if (fds[i].events & EPOLLOUT) {
	_LOG_DEBUG("%d became writable", fd);
	q.enq((*fwork)(fd, Work::write));
      }
      else {
	_LOG_DEBUG("???");
      }
      /* A handler might have set dowork to false; this will cause the
       * scheduler to fall through the poll loop right away. */
      if (!dowork) break;
    }
  }
  _LOG_INFO("scheduler retiring");
}

void Scheduler::_acceptcb::operator()()
{
  int acceptfd;
  if ((acceptfd = accept(fd, NULL, NULL))==-1) {
    // See "Unix Network Programming", 2nd ed., sec. 16.6 for rationale.
    if (errno == EWOULDBLOCK
	|| errno == ECONNABORTED
	|| errno == EPROTO
	|| errno == EINTR) {
      return;
    } else {
      throw SocketErr("accept", errno);
    }
  }
  // TODO: isn't this inherited already from fd?
  int flags;
  if ((flags = fcntl(acceptfd, F_GETFL))==-1)
    throw SocketErr("fcntl (F_GETFL)", errno);
  if (fcntl(acceptfd, F_SETFL, flags | O_NONBLOCK)==-1)
    throw SocketErr("fcntl (F_SETFL)", errno);

  sch.schedule((*fwork)(acceptfd, Work::read), true);
  _INC_STAT(accepts);
}

void Scheduler::_sigcb::operator()()
{
  struct signalfd_siginfo siginfo[sighandlers.size()];
  sighandler_map::iterator iter;
  ssize_t nread = read(fd, (void *)&siginfo, sizeof(siginfo));
  
  // There might be more than one pending signal.
  int i=0;
  while (nread > 0) {
    if ((iter = sighandlers.find(siginfo[i].ssi_signo)) != sighandlers.end()) {
      _LOG_INFO("got signal %s", strsignal(siginfo[i].ssi_signo));
      ((*iter).second)(0);
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
      _LOG_FATAL("sigaddset: %m %d", signo);
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
    _LOG_DEBUG();
    if (sigaction(signo, &act, NULL)==-1) {
      _LOG_FATAL("sigaction: %m");
      exit(1);
    }    
  }
}

void Scheduler::halt(int ignore)
{
  _LOG_INFO("scheduler halting");
  thescheduler->dowork = false;
  thescheduler->q.enq(NULL);
}
