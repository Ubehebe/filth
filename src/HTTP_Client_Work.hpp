#ifndef HTTP_CLIENT_WORK_HPP
#define HTTP_CLIENT_WORK_HPP

#include "HTTP_typedefs.hpp"
#include "HTTP_Work.hpp"
#include "Worker.hpp"

/* A general class for any "unit of work" that writes an HTTP request
 * and reads an HTTP response. */
class HTTP_Client_Work : public HTTP_Work
{
public:
  // The only function seen by the Worker.
  void operator()(Worker *w);
  HTTP_Client_Work(int fd, structured_hdrs_type &reqhdrs,
		   std::string const &req_body);
  virtual void browse_resp(structured_hdrs_type &resphdrs, std::string &resp_body);
  virtual ~HTTP_Client_Work();
};

#endif // HTTP_CLIENT_WORK_HPP
