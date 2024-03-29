#include <errno.h>
#include <unistd.h>

#include "logging.h"
#include "Work.hpp"

int Work::listenfd = -1;

Work::Work(int fd, mode m, bool deleteme, bool islisten)
  : fd(fd), m(m), deleteme(deleteme)
{
}

void Work::setlistenfd(int listenfd)
{
  Work::listenfd = listenfd;
}

Work::~Work()
{
  if (fd != listenfd) {
    if (close(fd)==-1)
      _LOG_INFO("close %d: %m", fd);
    else
      _LOG_DEBUG("close %d", fd);
  }
}

int Work::rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz)
{
  ssize_t nread;
  errno = 0;
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

int Work::rduntil(std::string &s, uint8_t *rdbuf, size_t rdbufsz, size_t &tord)
{
  ssize_t nread;
  while (tord > 0) {
    if ((nread = ::read(fd, (void *)rdbuf, rdbufsz-1))>0) {
      // Needed because I think operator<< calls strlen on right operand.
      rdbuf[nread] = '\0';
      tord -= nread;
      s.append(reinterpret_cast<char const *>(rdbuf));
    } else if (nread == -1 && errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
  return (tord == 0) ? 0 : errno;
}

int Work::wruntil(uint8_t const *&outbuf, size_t &towrite)
{
  ssize_t nwritten;
  while (towrite > 0) {
    if ((nwritten = ::write(fd, (void *) outbuf, towrite))>0) {
      outbuf += nwritten;
      towrite -= nwritten;
    } else if (nwritten == -1 && errno == EINTR) {
      continue;
    } else {
      break;
    }
  }
  return (towrite == 0) ? 0 : errno;
}
