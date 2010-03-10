#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "HTTP_Work.hpp"
#include "ServerErrs.hpp"

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;
FileCache *HTTP_Work::cache = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m)
{
  memset((void *)&cbuf, 0, cbufsz);
}

bool HTTP_Work::parse()
{
  // TODO!
}


void HTTP_Work::operator()()
{
  switch(m) {
  case Work::read: get_from_client(); break;
  case Work::write: put_to_client(); break;
  }
}

void HTTP_Work::get_from_client()
{
  ssize_t nread;
 /* Read until we would block.
  * My understanding is reading a socket will return 0
  * iff the peer hangs up. However, the scheduler already
  * checks the file descriptor for hangups, which is why
  * we don't check for nread==0 here. ??? */
  while (true) {
    if ((nread = ::read(fd, (void *)cbuf, cbufsz-1))>0)
      buf << cbuf;
    // Interrupted by a system call.
    else if (nread == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    throw SocketErr("read", errno);
  if (!parse())
    sch->reschedule(this);
  else
    ; // schedule write...?
}

void HTTP_Work::put_to_client()
{

}

void HTTP_mkWork::init(LockedQueue<Work *> *q, Scheduler *sch, FileCache *cache)
{
  HTTP_Work::q = q;
  HTTP_Work::sch = sch;
  HTTP_Work::cache = cache;
}

Work *HTTP_mkWork::operator()(int fd, Work::mode m)
{
  return new HTTP_Work(fd, m);
}
