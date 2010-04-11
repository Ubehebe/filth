#include "config.h" // for PACKAGE_NAME
#include "HTTP_CacheEntry.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

HTTP_CacheEntry::HTTP_CacheEntry(size_t szondisk,
				 size_t szincache,
				 time_t request_time,
				 time_t response_time,
				 time_t last_modified,
				 uint8_t *_buf,
				 content_coding enc)
  : szondisk(szondisk), szincache(szincache), request_time(request_time),
    response_time(response_time), last_modified(last_modified), _buf(_buf),
    use_max_age(false), max_age_value(0), enc(enc), date_value(response_time),
    age_value(0), expires_value(0)
{
}

HTTP_CacheEntry::~HTTP_CacheEntry()
{
  delete[] _buf;
}

bool HTTP_CacheEntry::response_is_fresh()
{
  return freshness_lifetime() > current_age();
}

void HTTP_CacheEntry::pushhdr(header h, std::string &val)
  {
    hdrlock.wrlock();
    hdrs[static_cast<int>(h)] = val; // copies val, which is what we want
    hdrlock.unlock();  
  }

void HTTP_CacheEntry::pushhdr(header h, char const *val)
  {
    hdrlock.wrlock();
    hdrs[static_cast<int>(h)] = val; // copies val, which is what we want
    hdrlock.unlock();  
  }

void HTTP_CacheEntry::pushstat(status stat)
{
  this->stat = stat;
}

uint8_t const *HTTP_CacheEntry::getbuf()
{
  return static_cast<uint8_t const *>(_buf);
}

ostream &operator<<(ostream &o, HTTP_CacheEntry &c)
{
  c.hdrlock.rdlock();
  o << HTTP_Version << ' ' << c.stat << CRLF
    << Content_Length << c.szincache << CRLF
    << Content_Encoding << c.enc << CRLF
    << Server << PACKAGE_NAME << CRLF;
  for (HTTP_CacheEntry::hdrmap_type::iterator it = c.hdrs.begin(); it != c.hdrs.end(); ++it)
    o << static_cast<header>(it->first) << it->second << CRLF;
  c.hdrlock.unlock();
  return o << CRLF; // The empty line separating headers and body
}
