#ifndef HTTP_CACHE_ENTRY_HPP
#define HTTP_CACHE_ENTRY_HPP

#include <string>
#include <unordered_map>
#include <time.h>

#include "HTTP_constants.hpp"
#include "Locks.hpp"

class HTTP_CacheEntry;

namespace HTTP_Origin_Server
{
  int request(std::string &path, HTTP_CacheEntry *result);
};

class HTTP_CacheEntry
{
  friend int HTTP_Origin_Server::request(std::string &, HTTP_CacheEntry *);
  typedef std::unordered_map<int, std::string> hdrmap_type;
  hdrmap_type hdrs;
  // Because multiple workers could have a pointer to the same cache entry
  Mutex hdrlock; 
  char *_buf;
  time_t date_value; // Date header
  time_t age_value; // Age header
  time_t expires_value; // Expires header
  bool use_max_age;
  time_t max_age_value; // max-age directive of Cache-Control header
  time_t request_time; // When the cache made this request
  time_t response_time; // When the cache received the response

  // These are all given in RFC 2616, secs. 13.2.3-4.
  time_t now();
  time_t corrected_received_age();
  time_t response_delay();
  time_t corrected_initial_age();
  time_t resident_time();
  time_t current_age();
  time_t freshness_lifetime();
public:
  bool response_is_fresh();
  size_t const sz;
  HTTP_CacheEntry(size_t sz) : sz(sz), _buf(new char[sz]) {}
  ~HTTP_CacheEntry() { delete[] _buf; }
};


#endif // HTTP_CACHE_ENTRY_HPP
