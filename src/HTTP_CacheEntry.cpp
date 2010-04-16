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
    use_expires(false), use_max_age(false), max_age_value(0), enc(enc),
    date_value(response_time), age_value(0), expires_value(0),
    omit_body(false), asif_uncompressed(false)
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

HTTP_CacheEntry &HTTP_CacheEntry::operator<<(pair<header, string &> p)
{
    metadata.wrlock();
    hdrs[static_cast<int>(p.first)] = p.second; // copies p.second
    metadata.unlock();
    return *this;
}

HTTP_CacheEntry &HTTP_CacheEntry::operator<<(pair<header, char const *> p)
{
  metadata.wrlock();
  hdrs[static_cast<int>(p.first)] = p.second; // copies p.second
  metadata.unlock();
  return *this;
}

HTTP_CacheEntry &HTTP_CacheEntry::operator<<(status &stat)
{
  this->stat = stat;
  return *this;
}

HTTP_CacheEntry &HTTP_CacheEntry::operator<<(HTTP_CacheEntry::manip m)
{
  metadata.wrlock();
  switch (m) {
  case as_HEAD:
    omit_body = true;
    break;
  case as_identity:
    asif_uncompressed = true;
  default:
    break;
  }
  metadata.unlock();
  return *this;
}

uint8_t const *HTTP_CacheEntry::getbuf()
{
  return static_cast<uint8_t const *>(_buf);
}

ostream &operator<<(ostream &o, HTTP_CacheEntry &c)
{
  c.metadata.rdlock();
  o << HTTP_Version << ' ' << c.stat << CRLF;
  if (c.omit_body)
    o << Content_Length << 0 << CRLF;
  else if (c.asif_uncompressed)
    o << Content_Length << c.szondisk << CRLF;
  else
    o << Content_Length << c.szincache << CRLF;

  o << Content_Encoding
    << ((c.asif_uncompressed) ? HTTP_constants::identity : c.enc) << CRLF
    << Server << PACKAGE_NAME << CRLF;
  for (HTTP_CacheEntry::hdrmap_type::iterator it = c.hdrs.begin(); it != c.hdrs.end(); ++it)
    o << static_cast<header>(it->first) << it->second << CRLF;
  c.omit_body = c.asif_uncompressed = false;
  c.metadata.unlock();
  return o << CRLF; // The empty line separating headers and body
}
