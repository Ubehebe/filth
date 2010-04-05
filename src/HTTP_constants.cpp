#include <string.h>

#include "HTTP_constants.hpp"
#include "HTTP_Parse_Err.hpp"
#include "logging.h"

using namespace std;

/* TODO: All the operator>>'s, and certainly all
 * the operator<<'s, work the same way, with some
 * minor details. Is there a way to templatize them
 * while still preserving the neat .def file structure? */

namespace HTTP_constants
{
  char const *HTTP_Version = "HTTP/1.1";
  char const *CRLF = "\r\n";

  size_t const num_status =
#define DEFINE_ME(blah1,blah2) +1
#include "HTTP_status.def"
#undef DEFINE_ME
    ;

  uint16_t const status_vals[] = {
#define DEFINE_ME(name,val) val,
#include "HTTP_status.def"
#undef DEFINE_ME
  };

  char const *status_strs[] = {
#define DEFINE_ME(name,val) #name,
#include "HTTP_status.def"
#undef DEFINE_ME
  };

  size_t const num_method = 
#define DEFINE_ME(ignore1, ignore2) +1
#include "HTTP_methods.def"
#undef DEFINE_ME
    ;

  char const *method_strs[] = {
#define DEFINE_ME(name, ignore) #name,
#include "HTTP_methods.def"
#undef DEFINE_ME
  };

  bool const method_is_implemented[] = {
#define DEFINE_ME(name, is_implemented) static_cast<bool>(is_implemented),
#include "HTTP_methods.def"
#undef DEFINE_ME
  };

  size_t const num_header = 
#define DEFINE_ME(ignore1, ignore2) +1
#include "HTTP_headers.def"
#undef DEFINE_ME
    ;

  char const *header_strs[] = {
#define DEFINE_ME(name, ignore) #name,
#include "HTTP_headers.def"
#undef DEFINE_ME
  };

  bool const header_is_implemented[] = {
#define DEFINE_ME(name, is_implemented) static_cast<bool>(is_implemented),
#include "HTTP_headers.def"
#undef DEFINE_ME
  };

  // Output is like "404 Not Found".
  ostream& operator<<(ostream &o, status &s)
  {
    o << status_vals[s] << ' ';
    char const *tmp = status_strs[s];
    while (*tmp) {
      o << ((*tmp == '_') ? ' ' : *tmp);
      ++tmp;
    }
    return o;
  }

  istream& operator>>(istream &i, method &m)
  {
    string tmp;
    i >> tmp;
    for (int j=0; j<num_method; ++j) {
      // Methods are case-sensitive; RFC 2616 sec. 5.1.1.
      if (tmp == method_strs[j]) {
	if (method_is_implemented[j]) {
	  m = static_cast<method>(j);
	  return i;
	} else throw HTTP_Parse_Err(Not_Implemented);
      }
    }
    throw HTTP_Parse_Err(Bad_Request);
  }

  ostream &operator<<(ostream &o, method &m)
  {
    return o << method_strs[m];
  }

  istream &operator>>(istream &i, header &h)
  {
    string tmp;
    i >> tmp;

    // Dirty trick to replace hyphens by underscores.
    string::size_type hyphen = -1;
    while ((hyphen = tmp.find('-', hyphen+1)) != tmp.npos) {
      tmp[hyphen] = '_';
    }
    tmp.erase(tmp.end()-1); // Because of the colon.

    for (int j=0; j<num_header; ++j) {
      if (tmp == header_strs[j]) {
	if (header_is_implemented[j]) {
	  h = static_cast<header>(j);
	  return i;
	} else throw HTTP_Parse_Err(Not_Implemented);
      }
    }
  }

  /* Value, not reference. Otherwise things like o << Content_Length
   * would be interpreted as putting an integer into the stream! */
  ostream &operator<<(ostream &o, header h)
  {
    // Dirty trick to replace underscores by hyphens
    char const *tmp = header_strs[h];
    while (*tmp) {
      o << ((*tmp == '_') ? '-' : *tmp);
      ++tmp;
    }
    // Tack on the colon and space.
    return o << ": ";
  }

  ostream &operator<<(ostream &o, header &h)
  {
    // Dirty trick to replace underscores by hyphens
    char const *tmp = header_strs[h];
    while (*tmp) {
      o << ((*tmp == '_') ? '-' : *tmp);
      ++tmp;
    }
    // Tack on the colon and space.
    return o << ": ";
  }
};
