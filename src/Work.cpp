#include <errno.h>
#include <unistd.h>

#include "logging.h"
#include "Work.hpp"

Work::Work(int fd, mode m)
  : fd(fd), m(m), deleteme(false)
{
}

Work::~Work()
{
  _LOG_DEBUG("close %d", fd);
  close(fd);
}

/* Read until we would block.
 * My understanding is reading a socket will return 0
 * iff the peer hangs up. However, the scheduler already
 * checks the file descriptor for hangups, which is why
 * we don't check for nread==0 here. ??? */
int Work::rduntil(std::stringstream &inbuf, char *rdbuf, size_t rdbufsz)
{
  ssize_t nread;
  while (true) {
    if ((nread = ::read(fd, (void *)rdbuf, rdbufsz-1))>0) {
      // Needed because I think operator<< calls strlen on right operand.
      rdbuf[nread] = '\0';
      inbuf << rdbuf;
    } else if (nread == -1 && errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
  return errno;
}

int Work::wruntil(char *&outbuf, size_t &towrite)
{
  ssize_t nwritten;
  while (true) {
    if ((nwritten = ::write(fd, (void *) outbuf, towrite))>0) {
      outbuf += nwritten;
      towrite -= nwritten;
    } else if (nwritten == -1 && errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
  return errno;
}
