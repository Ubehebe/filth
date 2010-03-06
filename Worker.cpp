#include <errno.h>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "Parsing.h"
#include "Scheduler.h"
#include "ServerErrs.h"
#include "Worker.h"

// Declare static members NULL so they can be initialized by the first worker.
LockedQueue<std::pair<int, char> > *Worker::q = NULL;
Scheduler *Worker::sch = NULL;
std::map<int, std::pair<std::stringstream *, Parsing *> > *Worker::state=NULL;
Parsing *Worker::pars = NULL;

Worker::Worker(LockedQueue<std::pair<int, char> > *_q, Scheduler *_sch,
	       std::map<int, std::pair<std::stringstream *, Parsing *> >
	       *_state, Parsing *_pars, time_t to_sec, size_t bufsz)
  : dowork(true), to_sec(to_sec), bufsz(bufsz)
{
  if (q == NULL) q = _q;
  if (sch == NULL) sch = _sch;
  if (state == NULL) state = _state;
  if (pars == NULL) pars = _pars;

  /* This will terminate the process without cleaning up any successfully
   * allocated workers (if any)...but I don't care. */
  if ((errno = pthread_create(&th, NULL, work_wrapper, (void *)this))!=0) {
    perror("pthread_create");
    _exit(1);
  }
}

// Annoying...needed for call to pthread_create
void *Worker::work_wrapper(void *worker)
{
  (*((Worker *)worker))();
}

/* TODO: it would be nice if the objects in the queue were just functions
 * we could immediately execute. But, I don't know enough about binding =) */
void Worker::operator()()
{
  ssize_t ndone;
  size_t buflen; // How much was actually written or read.
  int fd;
  std::stringstream *s;
  Parsing *p;
  char buf[bufsz];
  std::pair<int, char> job;
  std::map<int, std::pair<std::stringstream *, Parsing *> >::iterator it;
  
  while (dowork) {
    /* Do a timed wait so we can periodically check if dowork is false.
     * TODO: maybe it makes sense to have a huge to_sec as the default,
     * and when we want to shut down, make it tiny. */
    if (!q->timedwait_deq(job, to_sec))
      continue;
    fd = job.first;
    switch(job.second) {
    case 'r':
      /* If we are here, the nonblocking socket fd said it was ready
       * to be read. So, should it ever return an error? */
      if ((ndone = read(fd, (void *)buf, bufsz-1))==-1)
	throw SocketErr("read", errno);
      if ((it = state->find(fd)) == state->end()) {
	s = new std::stringstream();
	p = pars->mkParsing();
	(*state)[fd] = std::make_pair(s,p);
      }
      else {
	s = it->second.first;
	p = it->second.second;
      }
      /* This indicates an EOF, which might mean the client closed the
       * connection. This happens, e.g., when we have finished reading
       * a CGI server's response. */
      if (ndone == 0 && p->closeonread()) {
	if (close(fd)==-1)
	  throw SocketErr("close", errno);
	p->onclose();
	delete p;
	delete s;
	state->erase(fd);
	continue;
      }
      buf[ndone+1] = '\0';
      *s << buf;
      // Attempt to parse s. This should not throw any errors.
      *s >> *p;
      // Ready to write?
      if (p->parse_complete()) {
	s->str("");
	*s << *p;
	if (p->sched_counterop()) {
	  sch->reschedule(fd, 'w');
	}
      } 
      else {
	sch->reschedule(fd, 'r');
      }
      break;
    case 'w':
      /* If we are here, the nonblocking socket fd said it was ready
       * to be written to. */
      it = state->find(fd); // This should never fail
      s = it->second.first; 
      p = it->second.second;
      // Note that we don't need to null-terminate buf for writing...right?
      s->read(buf, bufsz);
      buflen = s->gcount();
      if ((ndone = write(fd, (void *) buf, buflen))==-1)
	throw SocketErr("write", errno);
      /* We didn't write all of buf, so rewind s to the right place
       * and schedule another write. */
      if (ndone < buflen) {
	if (s->eof())
	  s->seekg(buflen-ndone, std::ios::end);
	else
	  s->seekg(buflen-ndone, std::ios::cur);
	s->clear(); // ???
	sch->reschedule(fd, 'w');
      }
      // Nothing more to write?
      else if (s->eof() && p->closeonwrite()) {
	if (close(fd)==-1)
	  throw SocketErr("close", errno);
	p->onclose();
	delete p;
	delete s;
	state->erase(fd);
	continue;
      }
      else if (p->sched_counterop()) {
	sch->reschedule(fd, 'r');
      }
      // Still stuff to write, schedule another one.
      else {
	sch->reschedule(fd, 'w');
      }
      break;
    default: // Something other than 'r' or 'w'...
      std::unexpected();
    }
  }
}

// Blocks until the worker thread concludes.
void Worker::shutdown()
{
  dowork = false; 
  if ((errno = pthread_join(th, NULL))!=0)
    throw ResourceErr("pthread_join", errno);
  std::cerr << "worker retiring\n";
}

