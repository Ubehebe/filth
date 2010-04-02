#include <string>
#include <string.h>
#include <unistd.h>

#include "CacheWork.hpp"
#include "LockFreeQueue.hpp"
#include "logging.h"

using namespace std;

Scheduler *CacheWork::sch = NULL;
FileCache *CacheWork::cache = NULL;
Workmap *CacheWork::st = NULL;

CacheWork::CacheWork(int fd, Work::mode m)
  : Work(fd, m), path_written(false), resource(NULL)
{
}

CacheWork::~CacheWork()
{
  if (resource != NULL) {
    cache->release(path);
  }
  st->erase(fd);
}

void CacheWork::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = rduntil(inbuf, rdbuf, rdbufsz);
    if (err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    getline(inbuf, path, '\r');
    if (inbuf.peek() != '\n') {
      inbuf.clear();
      inbuf.str(path);
    }
    else {
      
      FileCache::cinfo *c;
      int err;
      c = cache->reserve(path, err);
      
      switch (err) {
      case 0:
	resource = c->buf;
	resourcesz = *const_cast<size_t *>(&c->sz);
	break;
      case ENOMEM:
      case EINVAL:
      case EACCES:
      case EISDIR:
      case ENOENT:
      case ESPIPE:
      default:
	/* In case of error, the client will see "(pathname)\r\n". */
	resource = NULL;
	resourcesz = 0;
	break;
      }
      statln = path + "\r\n";
      out = statln.c_str();
      outsz = statln.length();
      m = Work::write;
    }
    sch->reschedule(this);
    break;
  case Work::write:
    err = wruntil(out, outsz);
    if (err == EAGAIN || err == EWOULDBLOCK) {
      sch->reschedule(this);
    } else if (err != 0) {
      throw SocketErr("write", err);
    } else if (!path_written) {
      path_written = true;
      out = resource;
      outsz = resourcesz;
      sch->reschedule(this);
    } else {
      deleteme = true;
    }
    break;
  }
}
