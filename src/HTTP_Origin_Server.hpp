#ifndef HTTP_ORIGIN_SERVER_HPP
#define HTTP_ORIGIN_SERVER_HPP

#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include "HTTP_CacheEntry.hpp"
#include "logging.h"

/** \brief Encapsulates the server's interaction with the filesystem. */
namespace HTTP_Origin_Server
{
  /** \brief Get a file from disk in a form ready for caching.
   * \param path filepath
   * \param result On success, points to newly-allocated structure suitable
   * for caching; on failure, is NULL
   * \return 0 on success; on failure, EISDIR, ESPIPE, EACCES, ENOMEM,
   * and any error that stat or fopen could produce */
  int request(std::string &path, HTTP_CacheEntry *&result);
  /** \brief Replace (PUT) or append to (POST) a file on disk.
   * \param path filepath
   * \param contents contents to write
   * \param enc encoding of contents (because we have to decode contents
   * prior to writing)
   * \param mode "w" for PUT, "a" for POST
   * \return 0 on success; on failure, ESPIPE, EACCES, ENOMEM, ENOTSUP
   * (for unsupported content-codings), or any error that stat or fopen could
   * return.
   * \todo It might make sense to decode contents before passing it to
   * the origin server. */
  int putorpost(std::string const &path, std::string const &contents,
	  HTTP_constants::content_coding const &enc, char const *mode);
  /** \brief A wrapper around the unlink system call.
   * \param path filepath
   * \return 0 on success; on failure, any errno value that unlink(2) can
   * produce
   * \warning Servers that support this need to pay attention to their uid's
   * and gid's. Ideally the server should run with uid's and gid's different
   * from those carried by the files it interacts with, so it doesn't
   * accidentally unlink or overwrite something. */
  int unlink(std::string const &path);
  /** \brief Check whether a cache entry is "valid", as defined by the HTTP
   * standard.
   * \param path path to resource on disk
   * \param tocheck pointer to cache entry to validate
   * \return true if the cache entry is valid, false otherwise. */
  bool validate(std::string const &path, HTTP_CacheEntry *tocheck);
  /** \brief Turn a directory into a rudimentary snippet of HTML.
   * \param path path to directory
   * \param result on success, points to a newly allocated entry, suitable
   * for caching; on failure, is NULL
   * \return 0 on success; on failure, ENOMEM or any errno value that opendir
   * or readdir_r can produce.
   * \todo I'm not sure why this allocates a cache entry, as if we were going
   * to cache the result. Should directory pages be cacheable? */
  int dirtoHTML(std::string const &path, HTTP_CacheEntry *&result);
};

#endif // HTTP_ORIGIN_SERVER_HPP
