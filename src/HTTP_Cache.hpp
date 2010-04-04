#ifndef HTTP_CACHE_HPP
#define HTTP_CACHE_HPP

#include <string>
#include <time.h>
#include <unordered_map>

#include "Cache.hpp"
#include "HTTP_CacheEntry.hpp"
#include "Locks.hpp"

typedef Cache<std::string, HTTP_CacheEntry *> HTTP_Cache;

#endif // HTTP_CACHE_HPP
