#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "LockedQueue.h"
#include "Scheduler.h"
#include "ServerErrs.h"

Scheduler::Scheduler(LockedQueue<std::pair<int, char> > &q,
		     int pollsz, int maxevents, int to_ms)
  : dowork(false), working(false), maxevents(maxevents), q(q), to_ms(to_ms)
{
  if ((pollfd = epoll_create(pollsz))==-1)
    throw ResourceErr("epoll_create", errno);
}

// Annoying, needed for call to pthread_create
void *Scheduler::poll_wrapper(void *obj)
{
  ((Scheduler *)obj)->poll();
}


void Scheduler::start(int listenfd)
{
  this->listenfd = listenfd;

  /* listenfd will become readable when a connection is ready to be accepted.
   * Note that we don't simply call schedule because we don't want listenfd
   * to be one-shot. */

  struct epoll_event e;
  memset((void *) &e, 0, sizeof(e));
  e.data.fd = listenfd;
  e.events = EPOLLIN;
  if (epoll_ctl(pollfd, EPOLL_CTL_ADD, listenfd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_ADD)", errno);

  dowork = true;

  if ((errno = pthread_create(&th, NULL, poll_wrapper, (void *)this))!=0)
    throw ResourceErr("pthread_create", errno);

  working = true;

  // Blocks until we're done...
  if ((errno = pthread_join(th, NULL)) != 0)
    throw ResourceErr("pthread_join", errno);
}

void Scheduler::schedule(int fd, char mode)
{
  struct epoll_event e;
  memset((void *) &e, 0, sizeof(e));
  e.data.fd = fd;
  e.events = EPOLLONESHOT;
  if (mode == 'r')
    e.events |= EPOLLIN;
  else if (mode == 'w')
    e.events |= EPOLLOUT;

  if (epoll_ctl(pollfd, EPOLL_CTL_ADD, fd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_ADD)", errno);
}

void Scheduler::reschedule(int fd, char mode)
{
  struct epoll_event e;
  memset((void *) &e, 0, sizeof(e));
  e.data.fd = fd;
  e.events = EPOLLONESHOT; // N.B.
  if (mode == 'r')
    e.events |= EPOLLIN;
  else if (mode == 'w')
    e.events |= EPOLLOUT;

  if (epoll_ctl(pollfd, EPOLL_CTL_MOD, fd, &e)==-1)
    throw ResourceErr("epoll_ctl (EPOLL_CTL_MOD)", errno);
}
   
void Scheduler::poll()
{
  struct epoll_event ans[maxevents];
  int fd, nchanged, connfd, flags, so_err;
  socklen_t so_errlen = static_cast<socklen_t>(sizeof(so_err));

  while (dowork) {
    /* Holding a lock across epoll_wait is unacceptable, since this can delay
     * workers by up to to_ms. So how do we ensure pollfd is consistent?
     * Here is my current understanding. All the elements of pollfd except the
     * listening socket are "one-shot", which means they are disabled (but
     * present) once epoll_wait returns. (That is why reschedule uses the flag 
     * EPOLL_CTL_MOD instead of EPOLL_CTL_ADD.) Any worker may close a
     * file descriptor, which causes it automatically to disappear from pollfd.
     * So, what are the race conditions...? */

    /* TODO: if epoll_wait times out without anything happening, we can
     * close all the connections. But a better idea would be for each
     * socket to raise an error if it hasn't become readable in XXX time. Is
     * this doable? I know you can set the socket send and receive timeout, but
     * I think those only affect the timeouts on read and write calls. */
    if ((nchanged = epoll_wait(pollfd, ans, maxevents, to_ms))<0)
      throw ResourceErr("epoll_wait", errno);

    for (int i=0; i<nchanged; ++i) {
      fd = ans[i].data.fd;

      // Check for a pending error on the socket.
      // TODO: Don't generally know what to do in these cases.
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *) &so_err, &so_errlen)
	  ==-1)
	throw SocketErr("getsockopt", errno);
      else if (so_err != 0)
	throw SocketErr("socket error...what should be done?", errno);

      /* The listening socket became readable. This generally
       * means we can call accept() and it will not block.
       * See caveats at the end of section 16.6 of "Unix Network
       * Programming", 2nd ed. */
      else if (fd == listenfd) {
	// QUESTION: does the listening socket ever become writable?
	if (!(ans[i].events & EPOLLIN))
	  continue;
	// This sets connfd to be nonblocking without using fcntls.
	//if ((connfd = accept4(listenfd, NULL, NULL, SOCK_NONBLOCK))==-1) {
	// UGH! The CIMS machines don't have the accept4 syscall!!!
	if ((connfd = accept(listenfd, NULL, NULL))==-1) {
	  if (errno == EWOULDBLOCK
	      || errno == ECONNABORTED
	      || errno == EPROTO
	      || errno == EINTR)
	    continue;
	  else
	    throw SocketErr("accept", errno);
	}

	/* Make connected socket nonblocking.
	 * TODO: if we switch back to accept4 above,
	 * comment out the below. */
	int flags;
	if ((flags = fcntl(listenfd, F_GETFL))==-1)
	  throw SocketErr("fcntl (F_GETFL)", errno);
	if (fcntl(listenfd, F_SETFL, flags|O_NONBLOCK)==-1)
	  throw SocketErr("fcntl (F_SETFL)", errno);

	// Put the connected socket on the watch list.
	schedule(connfd, 'r');
      }

      /* Someone other than the listening socket became readable,
       * so just prepare a read. */
      else if (ans[i].events & EPOLLIN)
	q.enq(std::make_pair(fd,'r'));

      // The socket is writable, so prepare a write.
      else if (ans[i].events & EPOLLOUT)
	q.enq(std::make_pair(fd,'w'));
    }
  }
  working = false;
}

void Scheduler::shutdown()
{
  dowork = false;
  /* Busy wait until we know we're not doing anything.
   * The reason is that this gets called from the server destructor,
   * and we don't want to return from the server destructor (i.e.
   * free the server's stuff) until we know the scheduler isn't doing
   * anything; otherwise the poll loop could read freed memory.
   * 
   * Don't try to do a pthread_join; the server is already doing this
   * in operator(). */
  while (working)
    ;
  std::cerr << "scheduler retiring\n";
}


