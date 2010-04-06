#include <errno.h>
#include <unistd.h>

#include "logging.h"
#include "Work.hpp"

Work::Work(int fd, mode m)
  : fd(fd), m(m), deleteme(false), closeme(false)
{
  // N.B. the connection will NOT be closed by default!
}

Work::~Work()
{
  if (closeme) {
    _LOG_DEBUG("close %d", fd);
    close(fd);
  }
}

/* This call has "header" semantics, where we just keep reading bytes until
 * we would block. The idea is that after every block, some other component
 * checks the ostream to see if reading is complete (e.g. in HTTP, the last line
 * is blank). */
int Work::rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz)
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

int Work::rduntil(std::ostream &inbuf, uint8_t *rdbuf, size_t rdbufsz, size_t &tord)
{
  ssize_t nread;
  while (tord > 0) {
    if ((nread = ::read(fd, (void *)rdbuf, rdbufsz-1))>0) {
      // Needed because I think operator<< calls strlen on right operand.
      rdbuf[nread] = '\0';
      tord -= nread;
      inbuf << rdbuf;
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
  if (towrite == 0) return 0;
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
