#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "compression.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_Origin_Server.hpp"
#include "logging.h"

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
    int fd, ans;
    time_t req_t = ::time(NULL);

    errno = 0; // paranoid

  request_tryagain:
    if (stat(path.c_str(), &statbuf)==-1) {
      result = NULL;
      return errno;
    }
    else if (S_ISDIR(statbuf.st_mode)) {
      return EISDIR;
    }
    /* Note that ESPIPE covers both "socket" and "pipe". Will we eventually
     * need to discriminate between them? Is there any use for half-duplex
     * pipes when we can use full-duplex Unix domain sockets? */
    else if (S_ISSOCK(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode)) {
      result = NULL;
      return ESPIPE;
    }
    else if ((fd = open(path.c_str(), O_RDONLY)) ==-1) {
      result = NULL;
      return errno;
    }

    uint8_t *uncompressed;

    /* We use malloc/free for uncompressed and new/delete for compressed.
     * The reason for this atrocious admixture is just that valgrind reports an
     * (erroneous, I think) mismatched free/delete if we use new/delete for
     * uncompressed. But we can't use malloc/free for compressed, because
     * the compressed storage is deallocated in the HTTP_CacheEntry
     * destructor. */
    uncompressed
      = reinterpret_cast<uint8_t *>(malloc(statbuf.st_size * sizeof(uint8_t)));
    if (uncompressed == NULL) {
      close(fd);
      return ENOMEM;
    }

    size_t toread = statbuf.st_size;
    ssize_t nread;

    /* Get the file into memory with an old-fashioned blocking read.
     * TODO: replace with asynchronous I/O?
     *
     * The HTTP cache typically stores stuff already compressed. We
     * don't do compression yet because we might need to first operate
     * on the uncompressed file, e.g. compute a digest. */
    while (toread) {
      if ((nread = ::read(fd, reinterpret_cast<void *>(uncompressed), toread)) > 0) {
	toread -= nread;
	uncompressed += nread;
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
      free(uncompressed);
      goto request_tryagain;
    }
    close(fd);

    // Rewind pointer.
    uncompressed -= statbuf.st_size;

    uint8_t *compressed;
    size_t compressedsz = compression::compressBound(statbuf.st_size);

    try {
      compressed = new uint8_t[compressedsz];
    } catch (bad_alloc) {
      free(uncompressed);
      return ENOMEM;
    }
    
    if (compression::compress(reinterpret_cast<void *>(compressed),
			      compressedsz,
			      reinterpret_cast<void const *>(uncompressed), statbuf.st_size)) {
      result = new HTTP_CacheEntry(statbuf.st_size,
				   compressedsz,
				   req_t,
				   ::time(NULL),
				   statbuf.st_mtime,
				   compressed,
				   HTTP_constants::deflate);
      ans = 0;
    } else {
      delete compressed;
      ans = ENOMEM;
    }
    free(uncompressed);
    return ans;
  }

  int dirtoHTML(string &path, HTTP_CacheEntry *&result)
  {
    DIR *dirp;
    if ((dirp = opendir(path.c_str()))==NULL) {
      result = NULL;
      return errno;
    }
    // Copied from readdir_r man page. Ugh.
    struct dirent *entry;
    if ((entry = (struct dirent *) malloc(offsetof(struct dirent, d_name)
					  + pathconf(path.c_str(), _PC_NAME_MAX) + 1))
	==NULL) {
      result = NULL;
      return ENOMEM;
    }
    struct dirent *dir_res;
    int ans;
    string tmp = "<html><head></head><body>";

    while ((ans = readdir_r(dirp, entry, &dir_res))==0 && dir_res != NULL) {
      tmp += "<p><a href=\"/"
	+ path
	+ "/"
	+ dir_res->d_name + "\">"
	+ dir_res->d_name + "</a></p>";
    }
    closedir(dirp);
    free(entry);
    if (ans != 0) {
      result = NULL;
      return ans;
    }
    tmp += "</body></html>";
    char const *tmps = tmp.c_str();

    uint8_t *compressed;
    size_t srcsz = strlen(tmps);
    size_t compressedsz = compression::compressBound(srcsz);

    try {
      compressed = new uint8_t[compressedsz];
    } catch (bad_alloc) {
      return ENOMEM;
    }

    time_t now = ::time(NULL);
    
    if (compression::compress(reinterpret_cast<void *>(compressed),
			      compressedsz,
			      reinterpret_cast<void const *>(tmps), srcsz)) {
      result = new HTTP_CacheEntry(srcsz,
				   compressedsz,
				   now,
				   now,
				   now,
				   compressed,
				   HTTP_constants::deflate);
      ans = 0;
    } else {
      delete compressed;
      ans = ENOMEM;
    }
    return ans;
  }    
};
