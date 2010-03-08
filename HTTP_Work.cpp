#include <errno.h>
#include <iostream>
#include <unistd.h>

#include "HTTP_Work.hpp"
#include "ServerErrs.hpp"

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;

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
  case Work::read: get(); break;
  case Work::write: put(); break;
  }
}

void HTTP_Work::get()
{
  ssize_t nread;
 /* Read until we would block.
  * My understanding is reading a socket will return 0
  * iff the peer hangs up. However, the scheduler already
  * checks the file descriptor for hangups, which is why
  * we don't check for nread==0 here. ??? */
  while ((nread = ::read(fd, (void *)cbuf, cbufsz-1))>0)
    buf << cbuf;
  if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    throw SocketErr("read", errno);
  if (!parse())
    sch->reschedule(this);
  else
    ; // schedule write...?
}

void HTTP_Work::put()
{
  ssize_t cbuflen, nwritten;
  int err;
  while (true) {
    buf.read(cbuf, cbufsz);
    cbuflen = buf.gcount();
    // Write did not succeed.
    if ((nwritten = ::write(fd, (void *)&cbuf, cbuflen))==-1) {
      // Save errno.
      err = errno;
      // Rewind buf so as not to lose the data.
      buf.seekg(-cbuflen, std::ios::cur);
      if (err == EAGAIN || err == EWOULDBLOCK) {
	sch->reschedule(this);
	return;
      } else throw SocketErr("write", err);
    }
    // Write only partially succeeded.
    else if (nwritten < cbuflen) {
      // Rewind to not lose data.
      buf.seekg(nwritten-cbuflen, std::ios::cur);
      buf.clear(); // In case we reached eof
    }
    // Nothing more to write.
    else if (buf.eof()) {
      deleteme = true;
      return;
    }
  }
}

void HTTP_mkWork::init(LockedQueue<Work *> *q, Scheduler *sch)
{
  HTTP_Work::q = q;
  HTTP_Work::sch = sch;
}

Work *HTTP_mkWork::operator()(int fd, Work::mode m)
{
  return new HTTP_Work(fd, m);
}



