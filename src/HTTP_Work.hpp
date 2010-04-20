#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <stdint.h>
#include <sstream>
#include <string>
#include <vector>

#include "HTTP_constants.hpp"
#include "HTTP_typedefs.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

/** \brief Base class for work done by HTTP clients and servers.
 * \remarks A "client" writes a request and later reads a response, and a
 * "server" reads a request and later writes a response. The reason they have a
 * common base class is because we can use mostly the same buffers for reading
 * and writing. Note that this is pure virtual since Work's operator() is left
 * undefined. */
class HTTP_Work : public Work
{
public:
  /** \param fd open connection
   * \param m first activity the piece of work will do: should be read for
   * servers and write for clients. */
  HTTP_Work(int fd, Work::mode m);
  virtual ~HTTP_Work() {}
protected:
  HTTP_constants::status stat; //!< Not_Found, Internal_Server_Error, etc.
  static size_t const cbufsz = 1<<12; //!< Buffer size. Is this the right value?
  uint8_t cbuf[cbufsz]; //!< General-purpose C buffer. Don't use directly.
  /** \brief General-purpose input stream.
   * We need two streams because if the client pipelines requests, we might
   * read more than one whole request into inbuf. When this happens, we have
   * to preserve the request buffer for the next worker who might use it. */
  std::stringstream inbuf;
  /** \brief General-purpose output stream. */
  std::stringstream outbuf;

  bool inhdrs_done; //!< Have the input headers been read?
  bool outhdrs_done; //!< Have the output headers been written?
  /** \brief Only the inbound headers need to be stored in structured form,
   * since we might need to parse them; the outbound headers can be flat. */
  structured_hdrs_type inhdrs;
  std::string inbody; //!< Inbound body.
  /** \brief Value of the Content-Length request header for servers,
   * value of the Content-Length response header for clients */
  size_t inbody_sz;

  /* Before we start writing a request (client) or response (server), we set
   * out = outhdrs and outsz = outhdrs_sz. After we have finished writing
   * the headers, we set out = outbody and outsz = outbody_sz. */
  uint8_t const *outhdrs; //!< Pointer to output headers
  uint8_t const *outbody; //!< Pointer to output body
  uint8_t const *out; //!< Pointer to current output "stuff" (headers or body)
  size_t outhdrs_sz; //!< Size of output headers
  size_t outbody_sz; //!< Size of output body
  size_t outsz; //!< Size of current output "stuff" (headers or body)
};

#endif // HTTP_WORK_HPP
