#ifndef HTTP_PARSING_HPP
#define HTTP_PARSING_HPP

#include <iostream>

#include "HTTP_typedefs.hpp"

/** \brief Helper functions for parsing HTTP headers. */
namespace HTTP_parsing
{
  /** \brief The main parsing function.
   * Observe that this doesn't follow the stream operator patterns in C++;
   * they typically return a reference to the stream.
   * \param i stream containing HTTP headers (possibly malformed, incomplete,
   * etc.)
   * \param hdrs on success, contains headers in "structured" form
   * (just a vector)
   * \return true on success, false if we need to read more.
   * \note Throws an HTTP_oops with a value of Bad_Request if any line
   * is longer than HTTP_parsing::max_line_len */
  bool operator>>(std::istream &i, structured_hdrs_type &hdrs);
  /** \brief pushes the "structured" header object (a vector) to the stream.
   * \param o output stream
   * \param hdrs headers to push
   * \return reference to the output stream */
  std::ostream &operator<<(std::ostream &o,
				  structured_hdrs_type const &hdrs);
  /** \brief Set the length the beyond which the parsing function will throw
   * an exception
   * \param max_line_len maximum line length, in bytes */
  void setmaxes(size_t const &max_line_len);
};

#endif // HTTP_PARSING_HPP
