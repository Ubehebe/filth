#ifndef HTTP_SERVER_WORK_HPP
#define HTTP_SERVER_WORK_HPP

#include <config.h>
#include <iostream>
#include <string.h>

#include "HTTP_Client_Work.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_oops.hpp"
#include "HTTP_parsing.hpp"
#include "HTTP_Work.hpp"
#include "Worker.hpp"

/* A general class for any "unit of work" that reads an HTTP request
 * and writes an HTTP response. */
class HTTP_Server_Work : public HTTP_Work
{
public:
  // The only function seen by the Worker.
  void operator()(Worker *w);

  HTTP_Server_Work(int fd);
  virtual ~HTTP_Server_Work();
  virtual void async_setresponse(HTTP_Client_Work *assoc,
				 structured_hdrs_type const &resphdrs,
				 std::string const &respbody);
protected:
  // The main functions derived classes should override.
  virtual void prepare_response(structured_hdrs_type &reqhdrs,
				std::string const &reqbody,
				std::ostream &hdrs_dst,
				uint8_t const *&body,
				size_t &bodysz);
  virtual void on_parse_err(HTTP_constants::status &s, std::ostream &hdrs_dst);
  /* Derived classes need to override this in order to reset any state of a
   * "piece of work" at the end of servicing a request. The reason we just
   * don't delete this piece of work and get a new one later is because
   * HTTP connections are persistent by default. */
  virtual void reset();

  /* In some circumstances, pieces of work need to be able to become dormant,
   * not rescheduling themselves: for example, when we've finished parsing
   * a request, but have to wait for the arrival of some resource before we can
   * send out the response. (Eventually, when we replace blocking disk reads
   * by asynchronous I/O, we can use this too.) */
  bool nosch;

  /* Remember the worker who is working on our behalf, in case we need to
   * extract some state from it. */
  Worker *curworker;

  uint8_t *backup_body; // yikes

  // Convenience functions to be called from browse_req and prepare_response.
  void parsereqln(std::string &reqln, HTTP_constants::method &meth,
		  std::string &path, std::string &query);
  void parseuri(std::string &uri, std::string &path, std::string &query);
  std::string &uri_hex_escape(std::string &uri);
};

#endif // HTTP_SERVER_WORK_HPP
