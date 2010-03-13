#include <string.h>

#include "HTTP_constants.hpp"
#include "HTTP_Parse_Err.hpp"

/* TODO: All the operator>>'s, and certainly all
 * the operator<<'s, work the same way, with some
 * minor details. Is there a way to templatize them
 * while still preserving the neat .def file structure? */

namespace HTTP_constants
{
  char const *HTTP_Version = "HTTP/1.1";

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

  char *status_strs[] = {
#define DEFINE_ME(name,val) (char *)#name,
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

  char *header_strs[] = {
#define DEFINE_ME(name, ignore) (char *)#name,
#include "HTTP_headers.def"
#undef DEFINE_ME
  };

  bool const header_is_implemented[] = {
#define DEFINE_ME(name, is_implemented) static_cast<bool>(is_implemented),
#include "HTTP_headers.def"
#undef DEFINE_ME
  };

  void cpp_token_tinker(char **strs, size_t num, char from, char to)
  {
    char *tmp;
    for (int i=0; i<num; ++i) {
      tmp = strs[i];
      if ((tmp = strchr(tmp, from)) != NULL) {
	*tmp = to;
      }
    }
  }

  std::ostream& operator<<(std::ostream &o, status &s)
  {
    return o << status_strs[s];
  }

  std::istream& operator>>(std::istream &i, method &m)
  {
    std::string tmp;
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

  std::ostream &operator<<(std::ostream &o, method &m)
  {
    return o << method_strs[m];
  }

  std::istream &operator>>(std::istream &i, header &h)
  {
    std::string tmp;
    i >> tmp;

    for (int j=0; j<num_header; ++j) {
      if (tmp == header_strs[j]) {
	if (header_is_implemented[j]) {
	  h = static_cast<header>(j);
	  return i;
	} else throw HTTP_Parse_Err(Not_Implemented);
      }
    }
    throw HTTP_Parse_Err(Bad_Request);
  }

  std::ostream &operator<<(std::ostream &o, header &h)
  {
    return o << header_strs[h];
  }

};
