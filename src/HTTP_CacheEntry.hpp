#ifndef HTTP_CACHE_ENTRY_HPP
#define HTTP_CACHE_ENTRY_HPP

#include <sstream>
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

std::ostream &operator<<(std::ostream &o, HTTP_CacheEntry &c);

class HTTP_CacheEntry
{
  // friend so it can write _buf.
  friend int HTTP_Origin_Server::request(std::string &, HTTP_CacheEntry *);
  // friend so it can lock and unlock.
  friend std::ostream &operator<<(std::ostream &, HTTP_CacheEntry &);
  typedef std::unordered_map<HTTP_constants::header, std::string> hdrmap_type;
  hdrmap_type hdrs;
  std::string statln;
  // Because multiple workers could have a pointer to the same cache entry
  RWLock hdrlock; 
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
  // Should these return errors if they're already here?
  // TODO: turn into IO manipulators, so we can say e.g. entry << header << blah
  void pushhdr(HTTP_constants::header h, std::string &val)
  {
    hdrlock.wrlock();
    hdrs[h] = val; // copies val, which is what we want
    hdrlock.unlock();  
  }
  void pushstatln(std::istream &i)
  {
    hdrlock.wrlock();
    getline(i, statln, '\r');
    hdrlock.unlock();
  }
  char const *getbuf() { return static_cast<char const *>(_buf); }
};




#endif // HTTP_CACHE_ENTRY_HPP
