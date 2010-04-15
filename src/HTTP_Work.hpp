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

/* "Servers" read requests and write responses.
 * "Clients" write requests and read responses.
 * This base class contains data germane to both. Note that it is pure
 * virtual since the operator() is not defined. */
class HTTP_Work : public Work
{
public:
  HTTP_Work(int fd, Work::mode m);
  virtual ~HTTP_Work() {}
protected:
  HTTP_constants::status stat;
  // General-purpose buffers.
  static size_t const cbufsz = 1<<12;
  uint8_t cbuf[cbufsz];
  /* We need two buffers because if the client pipelines requests, we might
   * read more than one whole request into inbuf. When this happens, we have
   * to preserve the request buffer for the next worker who might use it. */
  std::stringstream inbuf, outbuf;

  bool inhdrs_done, outhdrs_done;
  /* Only the inbound headers need to be stored in structured form, since
   * we might need to parse them; the outbound headers can be flat bytes. */
  structured_hdrs_type inhdrs;
  std::string inbody;
  /* Value of the Content-Length request header for servers,
   * value of the Content-Length response header for clients */
  size_t inbody_sz;

  /* Before we start writing a request (client) or response (server), we set
   * out = outhdrs and outsz = outhdrs_sz. After we have finished writing
   * the headers, we set out = outbody and outsz = outbody_sz. */
  uint8_t const *outhdrs, *outbody, *out;
  size_t outhdrs_sz, outbody_sz, outsz;
};

#endif // HTTP_WORK_HPP
