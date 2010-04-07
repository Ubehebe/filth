#ifndef HTTP_CACHE_ENTRY_HPP
#define HTTP_CACHE_ENTRY_HPP

#include <sstream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <time.h>

#include "HTTP_constants.hpp"
#include "Locks.hpp"
#include "logging.h"

class HTTP_CacheEntry;

namespace HTTP_Origin_Server
{
  int request(std::string &path, HTTP_CacheEntry *&result);
};

std::ostream &operator<<(std::ostream &o, HTTP_CacheEntry &c);

class HTTP_CacheEntry
{
public:
  HTTP_CacheEntry(size_t szondisk,
		  size_t szincache,
		  time_t request_time,
		  time_t response_time,
		  time_t last_modified,
		  uint8_t *_buf,
		  HTTP_constants::content_coding c);
  ~HTTP_CacheEntry();
  bool response_is_fresh();
  HTTP_constants::content_coding const enc;
  size_t const szondisk, szincache;
  time_t last_modified; // Last-Modified header
  // TODO: turn into IO manipulators, so we can say e.g. entry << header << blah
  void pushhdr(HTTP_constants::header h, std::string &val);
  void pushhdr(HTTP_constants::header h, char const *val);
  void pushstat(HTTP_constants::status stat);
  uint8_t const *getbuf();
  bool use_max_age;
  time_t max_age_value; // max-age directive of Cache-Control header
private:
  // friend so it can write _buf.
  friend int HTTP_Origin_Server::request(std::string &, HTTP_CacheEntry *&);
  // friend so it can lock and unlock.
  friend std::ostream &operator<<(std::ostream &, HTTP_CacheEntry &);
  /* The key is an int instead of a HTTP_constants::header so that the compiler
   * won't complain about not having a hash function for headers. */
  typedef std::unordered_map<int, std::string> hdrmap_type;
  hdrmap_type hdrs;
  // Because multiple workers could have a pointer to the same cache entry
  RWLock hdrlock; 
  uint8_t *_buf;
  time_t date_value; // Date header
  time_t age_value; // Age header
  time_t expires_value; // Expires header

  time_t request_time; // When the cache made this request
  time_t response_time; // When the cache received the response

  HTTP_constants::status stat; // Generates status line

  // These are all given in RFC 2616, secs. 13.2.3-4.
  time_t now() { return ::time(NULL); }
  time_t corrected_received_age()
  {
    time_t tmp = now() - date_value;
    return (tmp > age_value) ? tmp : age_value;
  }
  time_t response_delay() { return response_time - request_time; }
  time_t corrected_initial_age()
  {
    return corrected_received_age() + response_delay();
  }
  time_t resident_time() { return now() - response_time; }
  // These guys are public because certain Cache-Control directives need them.
public:
  time_t current_age() { return corrected_initial_age() + resident_time(); }
  time_t freshness_lifetime()
  {
    return (use_max_age) ? max_age_value : expires_value - date_value;
  }
};

#endif // HTTP_CACHE_ENTRY_HPP
