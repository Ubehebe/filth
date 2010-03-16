#ifndef FILE_CACHE_TEST_HPP
#define FILE_CACHE_TEST_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

#include "FileCache.hpp"
#include "FindWork.hpp"
#include "LockedQueue.hpp"
#include "Scheduler.hpp"
#include "Thread.hpp"
#include "Work.hpp"

using namespace std;

struct randbytes
{
  char *buf;
  size_t nbytes;
  randbytes(size_t nbytes, unsigned int *seed)
    : nbytes(nbytes)
  {
    buf = new char[nbytes];
    for (size_t i=0; i<nbytes; ++i)
      buf[i] = rand_r(seed) % (1<<8);
  }
  ~randbytes() { delete buf; }
};

class CacheTester
{
protected:
  FileCache &cache;
public:
  virtual void work() = 0;
  CacheTester(FileCache &cache) : cache(cache) {}
};

class SimpleCacheTester : public CacheTester
{

public:
  SimpleCacheTester(FileCache &cache) : CacheTester(cache) {}
  void work()
  {
    unordered_map<string, randbytes *> stuff;
    unsigned int seed = time(NULL);
    size_t max = cache.getmax();
    char _template[10];

    _template[9] = '\0';
    int fd;
    // Try out files of different sizes.
    for (size_t sz=1; sz < max; sz <<= 1) {
      strncpy(_template, "tmpXXXXXX", 10);
      randbytes *r = new randbytes(sz, &seed);
      if ((fd = mkstemp(_template))==-1) {
	perror("mkstemp");
	abort();
      }
      size_t towrite = sz;
      ssize_t nwritten;
      char *buf = r->buf;
      while ((nwritten = write(fd, (void *)buf, towrite))>0) {
	buf += nwritten;
	towrite -= nwritten;
      }
      close(fd);
      stuff[string(_template)] = r;
    }
    for (int i=0; i < 10000; ++i) {
      int to_pick = rand_r(&seed) % stuff.size();
      size_t dummy;
      unordered_map<string, randbytes *>::iterator it = stuff.begin();
      while (to_pick--)
	++it;
      string tmp = it->first;
      char *compare = cache.reserve(tmp, dummy);
      if (compare == NULL)
	fprintf(stderr, "uh oh (NULL)");
      else if (memcmp((void *)compare, (void *)it->second->buf, it->second->nbytes))
	fprintf(stderr, "uh oh");
      cache.release(tmp);
    }
  }
};


class TestWork : public Work
{
public:
  void operator()() {}
  TestWork(int fd, Work::mode m) : Work(fd, m) {}
};

class TestFindWork : public FindWork
{
public:
  Work *operator()(int fd, Work::mode m)
  {
    return new TestWork(fd, m);
  }
};

void FileCacheTest()
{
  LockedQueue<Work *> q;
  TestFindWork fwt;
  Scheduler sch(q, fwt);
  FileCache cache(1<<20, fwt);
  SimpleCacheTester tester(cache);
  Thread<SimpleCacheTester> th(&tester, &SimpleCacheTester::work);
}

#endif // FILE_CACHE_TEST_HPP
