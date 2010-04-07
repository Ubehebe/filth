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
  // Data set by the base class that derived classes might need.
  typedef std::vector<std::string> reqhdrs_type;
  reqhdrs_type reqhdrs;
  string reqbody;
  size_t reqbodysz;
  status stat;

  // The main functions derived classes should override.
  virtual void browsehdrs() {}
  virtual void prepare_response();

  // Convenience functions
  virtual void parsereqln(method &meth, string &path, string &query);
  virtual void parseuri(std::string &uri, std::string &path, std::string &query);
  virtual string &uri_hex_escape(string &uri);

private:
  static Scheduler *sch;
  bool reqhdrs_done, resphdrs_done;
  static size_t const cbufsz = 1<<12; // =(

  // General-purpose parsing buffers.
  stringstream parsebuf;
  uint8_t cbuf[cbufsz];
  

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
