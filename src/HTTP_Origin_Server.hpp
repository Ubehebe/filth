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
  int request(std::string &path, HTTP_CacheEntry *&result);
  bool validate(std::string &path, HTTP_CacheEntry *tocheck);
};

#endif // HTTP_ORIGIN_SERVER_HPP
