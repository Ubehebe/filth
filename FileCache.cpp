#include <fcntl.h>
#include <stdlib.h>

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

bool FileCache::evict_cond(cinfo *c)
{
  return c->refcnt == 0;
}

bool FileCache::evict()
{
  bool ans = false;
  string s;
  cinfo *c;
  if (!toevict.nowait_deq(s))
    return ans;

  /* A file is put on the eviction list whenever its reference count
   * falls to 0; it is not automatically removed from the list when
   * its reference count increases. Thus, one file may appear more
   * than once on the list, and files with positive reference counts
   * may appear on the list. Therefore any file that is already gone
   * from the cache, and any file that has a positive reference count,
   * should just be ignored. */
  if (ans = h.popif(s, c, FileCache::evict_cond)) {
    __sync_sub_and_fetch(&cur, c->sz);
    delete c;
  }
  return ans;
}

/* N.B. this function passes path directly to the kernel.
 * The caller should already have done any needed validity checking
 * (e.g. making sure there are no ".."). */
char *FileCache::reserve(std::string &path, struct stat *statbuf)
{
  unordered_map<std::string, cinfo *>::iterator it;
  struct stat statbuf_backup;
  struct stat *sbp = (statbuf == NULL) ? &statbuf_backup : statbuf;
  
  cinfo *tmp;

  if (h.find(path, tmp)) {
    /* We have to synchronize the reference count update,
     * but a lock per cache entry sounds too cumbersome. So we
     * tell GCC to issue some fenced code. Clearly less portable
     * than a real lock.
     *
     * Note that if refcnt is 0 before adding, the file is on
     * the eviction list, but it shouldn't be evicted. Instead
     * of searching the eviction list here, we just check in the eviction
     * routine that the file about to be evicted indeed has refcnt==0. */
    __sync_fetch_and_add(&tmp->refcnt, 1);
    return tmp->buf;
  }

  int fd;

  // Try to stat and open file.
  if (stat(path.c_str(), sbp)==-1) {
    memset((void *)sbp, 0, sizeof(struct stat));
    return NULL;
  }
  // The cache should only deal with regular files
  if (!S_ISREG(sbp->st_mode))
    return NULL;
  if ((fd = open(path.c_str(), O_RDONLY))==-1)
    return NULL;

 reserve_tryagain:
  
  // Not enough room in the cache?
  if (__sync_add_and_fetch(&cur, sbp->st_size) > max) {
    __sync_sub_and_fetch(&cur, sbp->st_size);
    // If we were able to evict something try again.
    if (evict()) {
      goto reserve_tryagain;
    }
    // Otherwise just give up.
    else {
      close(fd);
      return NULL;
    }
  }

  try {
    tmp = new cinfo((size_t) sbp->st_size);
  }
  // If we weren't able to get that much memory from the OS, give up.
  catch (bad_alloc) {
    __sync_sub_and_fetch(&cur, sbp->st_size);
    close(fd);
    return NULL;
  }

  char *ctmp = tmp->buf;
  size_t toread = sbp->st_size;
  ssize_t nread;

  /* Get the file into memory with an old-fashioned blocking read.
   * TODO: replace with asynchronous I/O? */
  while (true) {
    if ((nread = read(fd, (void *) ctmp, toread)) > 0) {
      toread -= nread;
      ctmp += nread;
    }
    /* Interrupted by a system call. My current feeling is that the guys who
     * work with the cache (i.e., workers) should not also be the guys who
     * handle signals (i.e., schedulers or event managers), so that signals
     * should be masked here. But let's check anyway in case there is a
     * reason for such a configuration. */
    else if (nread == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  // Some other kind of error; start over.
  if (nread == -1) {
    perror("read");
    delete tmp;
    __sync_sub_and_fetch(&cur, sbp->st_size);
    goto reserve_tryagain;
  }
  close(fd);

  cinfo *already;
  if (h.insert(make_pair(path, tmp), already))
    return tmp->buf;
  /* While we were getting the file into memory, someone else
   * put it in cache! So we don't need our copy anymore. Wasteful
   * but simple. */
  else {
    delete tmp;
    __sync_sub_and_fetch(&cur, sbp->st_size);
    char *ans = already->buf;
    __sync_add_and_fetch(&already->refcnt, 1);
    return ans;
  }
}

void FileCache::release(std::string &path)
{
  cinfo *c;
  // This always succeeds...right?
  if (!h.find(path, c))
    abort();
  else if (__sync_sub_and_fetch(&c->refcnt, 1)==0)
    toevict.enq(path);
}

