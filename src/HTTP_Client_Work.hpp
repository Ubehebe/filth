#ifndef HTTP_CLIENT_WORK_HPP
#define HTTP_CLIENT_WORK_HPP

#include "HTTP_typedefs.hpp"
#include "HTTP_Work.hpp"
#include "Worker.hpp"

/** \brief Any "unit of work" that writes an HTTP request and reads an HTTP
 * response.
 * \remarks This class is in many ways complementary to HTTP_Server_Work.
 * It follows the same pattern of not exposing the intricate "main loop"
 * to derived classes, but rather allowing derived classes to override
 * certain hooks into the main loop. This class is much less developed
 * than HTTP_Server_Work because, after all, I am writing a server. */
class HTTP_Client_Work : public HTTP_Work
{
public:
  /** \brief "main loop" of the work object; the only thing seen by the worker.
   * \param w current worker, used to pass worker-specific state to the
   * work object, if needed */
  void operator()(Worker *w);
  /** \param fd open connection
   * \param reqhdrs request headers to send
   * \param req_body request body to send */
  HTTP_Client_Work(int fd, structured_hdrs_type const &reqhdrs,
		   std::string const &req_body);
  /** \brief What to do once we have read the response.
   * \param resphdrs the response headers
   * \param resp_body the response body */
  virtual void browse_resp(structured_hdrs_type const &resphdrs,
			   std::string const &resp_body) { }
  virtual ~HTTP_Client_Work();
};

#endif // HTTP_CLIENT_WORK_HPP
