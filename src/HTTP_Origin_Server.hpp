#ifndef HTTP_ORIGIN_SERVER_HPP
#define HTTP_ORIGIN_SERVER_HPP

#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include "HTTP_CacheEntry.hpp"
#include "logging.h"

/* This is a namespace instead of a class to emphasize that the origin
 * server is basically stateless and therefore thread-safe. It is a thin
 * abstraction around the filesystem. */
namespace HTTP_Origin_Server
{
  int request(std::string &path, HTTP_CacheEntry *result)
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

    HTTP_CacheEntry *tmp;

    try {
      tmp = new HTTP_CacheEntry(statbuf.st_size);
    }
    catch (std::bad_alloc) {
      close(fd);
      return ENOMEM;
    }

    char *ctmp = tmp->_buf;
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
      delete tmp;
      goto request_tryagain;
    }
    close(fd);
    return 0; // success
  }

};


#endif // HTTP_ORIGIN_SERVER_HPP
