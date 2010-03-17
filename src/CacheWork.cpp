#include "CacheWork.hpp"
#include "logging.h"

Scheduler *CacheWork::sch = NULL;
FileCache *CacheWork::cache = NULL;
Workmap *CacheWork::st = NULL;
LockedQueue<void *> CacheWork::store;

CacheWork::~CacheWork()
{
  /* Objects allocated through the dummy constructor have fd==-1.*/
  if (fd >= 0) {
    _LOG_DEBUG("close %d", fd);
    close(fd);
  }
  // If we're using the cache, tell it we're done.
  if (resource != NULL)
    cache->release(path);
  // If fd==-1, this will fail but that's OK.
  st->erase(fd);
}

void CacheWork::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = rduntil(inbuf, rdbuf, rdbufsz);
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    else if (err == 0) {
      inbuf >> path;
      if ((resource = cache->reserve(path, resourcesz))==NULL)
	resourcesz = 0;
      path += "\r\n";
      out = const_cast<char *>(path.c_str());
      outsz = path.length();
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

void *CacheWork::operator new(size_t sz)
{
  void *stuff;
  if (!store.nowait_deq(stuff))
    stuff = ::operator new(sz);
  return stuff;
}

void CacheWork::operator delete(void *ptr)
{
  store.enq(ptr);
}
