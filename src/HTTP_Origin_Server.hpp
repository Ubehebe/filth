#ifndef HTTP_ORIGIN_SERVER_HPP
#define HTTP_ORIGIN_SERVER_HPP

#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#include "HTTP_CacheEntry.hpp"
#include "logging.h"

/* This is a namespace instead of a class to emphasize that the origin
 * server is basically stateless. Currently the only system calls it makes are
 * open, stat, and close, all of which are required by POSIX to be thread-safe.
 * Therefore this namespace needs no synchronization. If you add to the
 * namespace, you had better be sure that this invariant is maintained. */
namespace HTTP_Origin_Server
{
  int request(std::string &path, HTTP_CacheEntry *&result);
  bool validate(std::string &path, HTTP_CacheEntry *tocheck);
};

#endif // HTTP_ORIGIN_SERVER_HPP
