#include <string>
#include <unistd.h>

#include "CacheWork.hpp"
#include "logging.h"

using namespace std;

Scheduler *CacheWork::sch = NULL;
FileCache *CacheWork::cache = NULL;
Workmap *CacheWork::st = NULL;
LockedQueue<void *> CacheWork::store;

CacheWork::CacheWork(int fd, Work::mode m)
  : Work(fd, m), path_written(false), resource(NULL)
{
}

CacheWork::~CacheWork()
{
  if (resource != NULL && resource[0] != '\0') // yikes
    cache->release(path);
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

    reserve_tryagain:      
      err = cache->reserve(path, resource, resourcesz);
      
      switch (err) {
      case 0: break;
      case ENOMEM:
      case EINVAL:
      case EACCES:
      case EISDIR:
      case ENOENT:
      case ESPIPE:
      default:
	resource = "\0";
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
      // Often get broken pipes here on heavy client testing. Why?
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

void *CacheWork::operator new(size_t sz)
{
  void *stuff;
  if (!store.nowait_deq(stuff))
    stuff = ::operator new(sz);
  return stuff;
}

void CacheWork::operator delete(void *ptr)
{
#ifdef INFO_MODE
  CacheWork *tmp = reinterpret_cast<CacheWork *>(ptr);
  if (tmp->deleteme)
    _LOG_INFO("double free detected! %d", tmp->fd);
#endif // INFO_MODE

  store.enq(ptr);
}
