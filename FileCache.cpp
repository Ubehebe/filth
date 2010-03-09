#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "FileCache.hpp"

using namespace std;

FileCache::cinfo::cinfo(size_t sz)
  : sz(sz), refcnt(1)
{
  buf = new char[sz];
}

FileCache::cinfo::~cinfo()
{
  delete[] buf;
}

bool FileCache::evict()
{
  string *s;
  if (!toevict.nowait_deq(s))
    return false;
  unordered_map<string, cinfo *>::iterator it;
  lock.wrlock();
  /* TODO: explain these ifs! */
  if ((it = c.find(*s)) != c.end() && (*it).second->refcnt == 0) {
    __sync_sub_and_fetch(&cur, (*it).second->sz);
    delete (*it).second;
    c.erase(it);
  }
  lock.unlock();
  return true;
}

char *FileCache::reserve(std::string &path)
{
  // Do NOT allow people to get out of the server's mount dir!
  if (path.find("..") != string::npos)
    return NULL;

  unordered_map<std::string, cinfo *>::iterator it;
  struct stat statbuf;

  lock.rdlock();
  it = c.find(path);
  lock.unlock();

  if (it != c.end()) {
    /* We have to synchronize the reference count update,
     * but a lock per cache entry sounds too cumbersome. So we
     * tell GCC to issue some fenced code. Clearly less portable
     * than a real lock. */
    __sync_fetch_and_add(&(*it).second->refcnt, 1);
    return (*it).second->buf;
  }

  int fd;

  if (stat(path.c_str(), &statbuf)==-1
      || (fd = open(path.c_str(), O_RDONLY))==-1)
    return NULL;

 reserve_tryagain:
  
  if (__sync_add_and_fetch(&cur, statbuf.st_size) > max) {
    __sync_sub_and_fetch(&cur, statbuf.st_size);
    if (evict())
      goto reserve_tryagain;
    else 
      return NULL;
  }

  cinfo *tmp = new cinfo(statbuf.st_size);
  char *ctmp = tmp->buf;
  size_t toread = statbuf.st_size;
  ssize_t nread;
  while ((nread = read(fd, (void *) ctmp, toread)) > 0) {
    toread -= nread;
    ctmp += nread;
  }
  if (nread == -1) {
    perror("read");
    delete tmp;
    __sync_sub_and_fetch(&cur, statbuf.st_size);
    goto reserve_tryagain;
  }
  lock.wrlock();
  it = c.find(path);
  if (it == c.end()) {
    c[path] = tmp;
    lock.unlock();
    return tmp->buf;
  }
  else {
    delete tmp;
    __sync_sub_and_fetch(&cur, statbuf.st_size);
    char *ans = (*it).second->buf;
    (*it).second->refcnt++;
    lock.unlock();
    return ans;
  }
}

void FileCache::release(std::string &path)
{
  bool doenq;
  unordered_map<string, cinfo *>::iterator it;  
  lock.rdlock();
  it = c.find(path);
  doenq = (__sync_sub_and_fetch(&(*it).second->refcnt, 1)==0);
  lock.unlock();
  if (doenq)
    toevict.enq(const_cast<string *>(&((*it).first)));
}

int main()
{

}
