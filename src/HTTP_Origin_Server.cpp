#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "gzip.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_Origin_Server.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

namespace HTTP_Origin_Server
{
  bool validate(string const &path, HTTP_CacheEntry *tocheck)
  {
    struct stat statbuf;
    return tocheck != NULL
      && stat(path.c_str(), &statbuf) == 0
      && statbuf.st_mtime <= tocheck->last_modified;
  }

  // Ya best make sure this is secure!
  int unlink(string const &path)
  {
    return unlink(path.c_str());
  }

  int put(string const &path, string const &contents,
	  content_coding const &enc, char const *mode)
  {
    size_t towrite = contents.length();
    _LOG_DEBUG("towrite %d", towrite);
    char *tmp = NULL;
    size_t uncompressed_sz;
    int ans;
    switch (enc) {
    case HTTP_constants::identity:
      tmp = const_cast<char *>(contents.c_str());
      break;
    case HTTP_constants::gzip:
      tmp = reinterpret_cast<char *>(gzip::uncompress(uncompressed_sz,
						      contents.c_str(), towrite));
      if (tmp == NULL)
	return ENOMEM;
      towrite = uncompressed_sz;
      break;
    default:
      return ENOTSUP;
    }

    FILE *fp;
    
    // We are not gonna allow clients to put files that don't already exist.
    if ((fp = fopen(path.c_str(), "r"))==NULL && errno == ENOENT) {
      if (enc != HTTP_constants::identity)
	delete[] tmp;
      return ENOENT;
    } else {
      fclose(fp);
    }

  put_tryagain:

    if ((fp = fopen(path.c_str(), mode))==NULL) {
      if (enc != HTTP_constants::identity)
	delete[] tmp;
      return errno;
    }

    if (ftrylockfile(fp)!=0) {
      if (enc != HTTP_constants::identity)
	delete[] tmp;
      fclose(fp);
      return ETXTBSY;
    }

    size_t nwritten;
    while (towrite) {
      if ((nwritten = fwrite_unlocked(reinterpret_cast<void const *>(tmp),
				      1, towrite, fp))>0) {
	towrite -= nwritten;
	tmp += nwritten;
      }
    }
    if (ferror(fp)) {
      clearerr(fp);
      funlockfile(fp);
      fclose(fp);
      goto put_tryagain;
    }
    funlockfile(fp);
    fclose(fp);
    if (enc != HTTP_constants::identity)
      delete[] tmp;
    return 0;
  }

  int request(string &path, HTTP_CacheEntry *&result)
  {
    struct stat statbuf;
    int ans;
    FILE *fp;
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
    else if ((fp = fopen(path.c_str(), "r"))==NULL) {
      result = NULL;
      return errno;
    }
    else if (ftrylockfile(fp)!=0) {
      fclose(fp);
      result = NULL;
      return ETXTBSY; // "text file busy"
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
      funlockfile(fp);
      fclose(fp);
      return ENOMEM;
    }

    size_t toread = statbuf.st_size;
    size_t nread;

    /* Get the file into memory with an old-fashioned blocking read.
     * TODO: replace with asynchronous I/O? */
    while (toread) {
      if ((nread = fread_unlocked(reinterpret_cast<void *>(uncompressed),
				  1, toread, fp)) > 0) {
	toread -= nread;
	uncompressed += nread;
      }
    }
    // Some other kind of error; start over.
    if (ferror(fp)) {
      clearerr(fp);
      funlockfile(fp);
      fclose(fp);
      free(uncompressed);
      goto request_tryagain;
    }
    funlockfile(fp);
    fclose(fp);

    // Rewind pointer.
    uncompressed -= statbuf.st_size;

    uint8_t *compressed;
    size_t compressedsz = gzip::compressBound(statbuf.st_size);

    try {
      compressed = new uint8_t[compressedsz];
    } catch (bad_alloc) {
      free(uncompressed);
      _LOG_DEBUG();
      return ENOMEM;
    }
    
    if (gzip::compress(reinterpret_cast<void *>(compressed),
		       compressedsz,
		       reinterpret_cast<void const *>(uncompressed), statbuf.st_size)) {
      result = new HTTP_CacheEntry(statbuf.st_size,
				   compressedsz,
				   req_t,
				   ::time(NULL),
				   statbuf.st_mtime,
				   compressed,
				   HTTP_constants::gzip);
      ans = 0;
    } else {
      delete compressed;
      ans = ENOMEM;
    }
    free(uncompressed);
    return ans;
  }

  int dirtoHTML(string const &path, HTTP_CacheEntry *&result)
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
    size_t compressedsz = gzip::compressBound(srcsz);

    try {
      compressed = new uint8_t[compressedsz];
    } catch (bad_alloc) {
      return ENOMEM;
    }

    time_t now = ::time(NULL);
    
    if (gzip::compress(reinterpret_cast<void *>(compressed),
		       compressedsz,
		       reinterpret_cast<void const *>(tmps), srcsz)) {
      result = new HTTP_CacheEntry(srcsz,
				   compressedsz,
				   now,
				   now,
				   now,
				   compressed,
				   HTTP_constants::gzip);
      ans = 0;
    } else {
      delete compressed;
      ans = ENOMEM;
    }
    return ans;
  }    
};
