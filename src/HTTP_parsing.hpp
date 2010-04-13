#ifndef HTTP_PARSING_HPP
#define HTTP_PARSING_HPP

#include <iostream>

#include "HTTP_typedefs.hpp"

// Returns true if we're done parsing, false else.
bool operator>>(std::istream &i, structured_hdrs_type &hdrs);

std::ostream &operator<<(std::ostream &o, structured_hdrs_type &hdrs);

#endif // HTTP_PARSING_HPP
