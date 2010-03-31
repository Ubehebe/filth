#include <string>
#include <unistd.h>

#include "CacheWork.hpp"
#include "LockFreeQueue.hpp"
#include "logging.h"

using namespace std;

Scheduler *CacheWork::sch = NULL;
FileCache *CacheWork::cache = NULL;
Workmap *CacheWork::st = NULL;

CacheWork::CacheWork(int fd, Work::mode m)
  : Work(fd, m), path_written(false), has_reserved(false), resource(NULL)
{
}

CacheWork::~CacheWork()
{
  if (has_reserved) {
    _LOG_DEBUG("releasing %s", path.c_str());
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

      err = cache->reserve(path, resource, resourcesz);
      
      switch (err) {
      case 0:
	has_reserved = true;
	break;
      case ENOMEM:
      case EINVAL:
      case EACCES:
      case EISDIR:
      case ENOENT:
      case ESPIPE:
      default:
	errno = err;
	/* In case of error, the client will see "(pathname)\r\n\0". */
	resource = const_cast<char *>("\0");
	resourcesz = 1;
	break;
      }
      statln = path + "\r\n";
      out = const_cast<char *>(statln.c_str());
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
      _LOG_DEBUG("finished sending %s to %d", path.c_str(), fd);
      deleteme = true;
    }
    break;
  }
}
