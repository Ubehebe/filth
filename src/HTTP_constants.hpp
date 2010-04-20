#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <iostream>
#include <stdint.h>
#include <sys/types.h>
#include <string>

/** \brief Namespace for most of the "enum"s defined by the HTTP standard. */
namespace HTTP_constants
{
  /** \brief Not_Found, Internal_Server_Error, etc. */
  enum status {
#define DEFINE_ME(name,val) name,
#include "HTTP_status.def"
#undef DEFINE_ME
  };

  /** \brief GET, PUT, etc. */
  enum method
    {
#define DEFINE_ME(name) name,
#include "HTTP_methods.def"
#undef DEFINE_ME
    };

  /** \brief Cache-Control, Date, etc.
   * \note Includes ALL the headers: general, request, response, entity,
   * even though not all of these are allowed to appear in a request or a
   * response. */
  enum header
    {
#define DEFINE_ME(name) name,
#include "HTTP_headers.def"
#undef DEFINE_ME
    };

  /** \brief gzip, identity, etc. */
  enum content_coding
    {
#define DEFINE_ME(name) name,
#include "HTTP_content_codings.def"
#undef DEFINE_ME
    };

  /** \brief Push a status (output is e.g. "404 Not Found")
   * \param o output stream
   * \param s Not_Found, Internal_Server_Error, etc.
   * \return reference to stream
   * \note We don't need an operator>> for statuses (yet!) */
  std::ostream &operator<<(std::ostream &o, status &s);
  /** \brief Parse a method.
   * \param i input stream
   * \param m on success, contains method
   * \return reference to stream
   * \note throws an HTTP_oops with status of Bad_Request if parsing fails */
  std::istream &operator>>(std::istream &i, method &m);
  /** \brief Push a method (output is e.g. "PUT")
   * \param o output stream
   * \param m method to push
   * \return reference to stream */
  std::ostream &operator<<(std::ostream &o, method &m);
  /** \brief Parse a header name.
   * \param i input stream
   * \param h on success, contains header type
   * \return reference to stream
   * \note throws an HTTP_oops with a value of Bad_Request if parsing fails
   * \note Header lines have the general form Name: value. If a statement
   * like \code i >> h; \endcode succeeds, i still contains value. */
  std::istream &operator>>(std::istream &i, header &h);
  /** \brief Push a header name (output is e.g. "Server: ")
   * \param o output stream
   * \param h header
   * \return reference to stream
   * \note h is a value, not a reference. Otherwise statements like
   * \code o << Content_Length; \endcode would be interpreted as pushing
   * an integer into the stream! */
  std::ostream &operator<<(std::ostream &o, header h);
  /** \brief Push a content-coding (output is e.g. "gzip")
   * \param o output stream
   * \param c content-coding
   * \return reference to stream
   * \note No operator>> for content codings; we do a strcasecmp on the
   * header. */
  std::ostream &operator<<(std::ostream &o, content_coding c);

  // These guys are all defined in HTTP_constants.cpp to avoid linker errors.
  extern char const *HTTP_Version; //!< "HTTP/1.1"
  extern char const *CRLF; //!< "\r\n"
  extern size_t const num_status; //!< How many HTTP statuses there are
  extern uint16_t const status_vals[]; //!< e.g. 404 for Not_Found
  extern char const *status_strs[]; //!< e.g. "Not Found" for Not_Found
  extern size_t const num_method; //!< How many HTTP methods there are (8?)
  extern char const *method_strs[]; //!< e.g. "GET"
  extern size_t const num_header; //!< How many HTTP headers there are (a lot)
  extern size_t const reqln; //!< hack
  extern char const *header_strs[]; //!< e.g. "Cache-Control"
  extern size_t const num_content_coding; //!< Number of HTTP content encodings
  extern char const *content_coding_strs[]; //!< e.g. "gzip"
};

#endif // HTTP_CONSTANTS_HPP
