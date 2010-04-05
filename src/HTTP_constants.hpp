#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <iostream>
#include <stdint.h>
#include <sys/types.h>
#include <string>

namespace HTTP_constants
{
  // 404, etc.
  enum status {
#define DEFINE_ME(name,val) name,
#include "HTTP_status.def"
#undef DEFINE_ME
  };

  // GET, PUT, etc.
  enum method
    {
#define DEFINE_ME(name, ignore) name,
#include "HTTP_methods.def"
#undef DEFINE_ME
    };

  // General, entity, and request headers.
  enum header
    {
#define DEFINE_ME(name, ignore) name,
#include "HTTP_headers.def"
#undef DEFINE_ME
    };

  // Don't need an operator>> for statuses.
  std::ostream &operator<<(std::ostream &i, status &s);
  std::istream &operator>>(std::istream &i, method &m);
  std::ostream &operator<<(std::ostream &o, method &m);
  std::istream &operator>>(std::istream &i, header &h);
  /* Value, not reference. Otherwise things like o << Content_Length
   * would be interpreted as putting an integer into the stream! */
  std::ostream &operator<<(std::ostream &o, header h);

  // These guys are all defined in HTTP_constants.cpp to avoid linker errors.
  extern char const *HTTP_Version;
  extern char const *CRLF;
  extern size_t const num_status;
  extern uint16_t const status_vals[];
  extern char const *status_strs[];
  extern size_t const num_method;
  extern char const *method_strs[];
  extern bool const method_is_implemented[];
  extern bool const header_is_implemented[];
  extern size_t const num_header;
  extern char const *header_strs[];
};

#endif // HTTP_CONSTANTS_HPP
