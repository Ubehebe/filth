#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <config.h>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>

#include "HTTP_constants.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_parsing.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

using namespace std;
using namespace HTTP_constants;

/* A general class for any "unit of work" that reads an HTTP request
 * and writes an HTTP response. */
class HTTP_Work : public Work
{
public:
  // The only function seen by the Worker.
  void operator()();
  HTTP_Work(int fd, Work::mode m);
  static void setsch(Scheduler *sch) { HTTP_Work::sch = sch; }

protected:
  typedef std::vector<std::string> req_hdrs_type;

  /* The need for this shouldn't be great. If there's a parse error,
   * just throw an HTTP_Parse_Err with the right status code and it'll
   * get picked up automatically. This is more for situations when
   * we want to change the status from OK although there's no error. */
  status stat;

  /* The main functions derived classes should override.
   * The reason reqhdrs isn't const in browse_req is to allow for small
   * in-place transformations like changing case, which is handy for doing
   * case-insensitive comparisons, as required by certain parts of the HTTP
   * grammar. */
  virtual void browse_req(req_hdrs_type &reqhdrs, std::string const &req_body) {}
  virtual void prepare_response(stringstream &hdrs, uint8_t const *&body,
				size_t &bodysz);
  /* If anything in the above two functions throws a parse error, the base
   * class will use this function to construct an error message. */
  virtual void on_parse_err(status &s, stringstream &hdrs, uint8_t const *&body,
			    size_t &bodysz);
  /* Derived classes need to override this in order to reset any state of a
   * "piece of work" at the end of servicing a request. The reason we just
   * don't delete this piece of work and get a new one later is because
   * HTTP connections are persistent by default. */
  virtual void reset();

  // Convenience functions to be called from browsehdrs and prepare_response.
  void parsereqln(req_hdrs_type &req_hdrs, method &meth, string &path,
		  string &query);
  void parseuri(std::string &uri, std::string &path, std::string &query);
  string &uri_hex_escape(string &uri);

private:
  static Scheduler *sch;

  bool req_hdrs_done, resp_hdrs_done;
  static size_t const cbufsz = 1<<12; // =(

  // General-purpose parsing buffers.
  stringstream parsebuf;
  uint8_t cbuf[cbufsz];

  req_hdrs_type req_hdrs;
  string req_body;
  size_t req_body_sz;

  // Pointers to buffers involved in writing

  // Points to buffer containing response headers (never NULL)
  uint8_t const *resp_hdrs;
  size_t resp_hdrs_sz;

  // Points to buffer containing response body (can be NULL)
  uint8_t const *resp_body;
  size_t resp_body_sz;

  /* Before we start writing a response, we set
   * out = resp_hdrs and outsz = resp_hdrs_sz. After we have finished writing
   * the headers, we set
   * out = resp_body and outsz = resp_body_sz. */
  uint8_t const *out;
  size_t outsz;
};







#endif // HTTP_WORK_HPP
