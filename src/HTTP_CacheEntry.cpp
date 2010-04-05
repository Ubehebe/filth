#include "HTTP_CacheEntry.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

ostream &operator<<(ostream &o, HTTP_CacheEntry &c)
{
  c.hdrlock.rdlock();
  o << HTTP_Version << ' ' << c.stat << CRLF
    << Content_Length << c.sz << CRLF;
  for (HTTP_CacheEntry::hdrmap_type::iterator it = c.hdrs.begin(); it != c.hdrs.end(); ++it)
    o << static_cast<header>(it->first) << it->second << CRLF;
  c.hdrlock.unlock();
  return o << CRLF; // The empty line separating headers and body
}
