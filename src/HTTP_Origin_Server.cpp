#include "HTTP_Origin_Server.hpp"

using namespace std;

namespace HTTP_Origin_Server
{
  bool validate(string &path, HTTP_CacheEntry *tocheck)
  {
    struct stat statbuf;
    return tocheck != NULL
      && stat(path.c_str(), &statbuf) == 0
      && statbuf.st_mtime <= tocheck->last_modified;
  }

  int request(string &path, HTTP_CacheEntry *&result)
  {
    struct stat statbuf;
    int fd;

  request_tryagain:
    if (stat(path.c_str(), &statbuf)==-1) {
      result = NULL;
      return errno;
    }
    // Will eventually want to replace this with a custom page.
    else if (S_ISDIR(statbuf.st_mode)) {
      result = NULL;
      return EISDIR;
    }
    else if (S_ISSOCK(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)) {
      result = NULL;
      return ESPIPE;
    }
    else if ((fd = open(path.c_str(), O_RDONLY)) ==-1) {
      result = NULL;
      return errno;
    }

    try {
      result = new HTTP_CacheEntry(statbuf.st_size, statbuf.st_mtime);
    }
    catch (bad_alloc) {
      close(fd);
      return ENOMEM;
    }

    char *ctmp = result->_buf;
    size_t toread = statbuf.st_size;
    ssize_t nread;

    /* Get the file into memory with an old-fashioned blocking read.
     * TODO: replace with asynchronous I/O? */
    while (toread) {
      if ((nread = ::read(fd, (void *) ctmp, toread)) > 0) {
	toread -= nread;
	ctmp += nread;
      }
      else if (nread == -1 && errno == EINTR)
	continue;
      else
	break;
    }
    // Some other kind of error; start over.
    if (nread == -1) {
      _LOG_INFO("read %s: %m, starting read over", path.c_str());
      close(fd);
      delete result;
      goto request_tryagain;
    }
    close(fd);
    return 0; // success
  }
};
