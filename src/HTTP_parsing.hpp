#ifndef HTTP_PARSING_HPP
#define HTTP_PARSING_HPP

#include <iostream>

#include "HTTP_typedefs.hpp"

namespace HTTP_parsing
{
  // Returns true if we're done parsing, false else.
  bool operator>>(std::istream &i, structured_hdrs_type &hdrs);
  std::ostream &operator<<(std::ostream &o,
				  structured_hdrs_type const &hdrs);
  void setmaxes(size_t const &max_line_len);
};

#endif // HTTP_PARSING_HPP
