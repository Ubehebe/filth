#ifndef HTTP_SERVER_WORK_HPP
#define HTTP_SERVER_WORK_HPP

#include <config.h>
#include <iostream>
#include <string.h>

#include "HTTP_Client_Work.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_oops.hpp"
#include "HTTP_Work.hpp"
#include "Worker.hpp"

/** \brief Any "unit of work" that reads an HTTP request and writes an HTTP
 * response.
 * \remarks This class guides a unit of work along the "assembly line" from
 * reading a request to writing a response. This "assembly line" is intricate,
 * because it has to deal with incomplete requests, pipelined requests,
 * interrupted responses, etc. Accordingly, the operator() that implements the
 * "assembly line" cannot be overriden by derived classes. Instead, this class
 * provides a number of virtual methods that are called at the appropriate times
 * in the "assembly line". These virtual methods have a somewhat odd pattern.
 * Derived classes can override their definition but should never actually call
 * them; the "main loop" of the work object is fixed in this class and calls the
 * virtual methods at the right times. */
class HTTP_Server_Work : public HTTP_Work
{
public:
  /** \brief "main loop" of the work object; the only thing seen by the worker.
   * \param w current worker, used to pass worker-specific state to the
   * work object, if needed */
  void operator()(Worker *w);
  /** \param fd open connection */
  HTTP_Server_Work(int fd);
  virtual ~HTTP_Server_Work();
  /** \brief Set limits on the length of the request URI and body.
   * \param max_req_uri maximum request URI length, bytes
   * \param max_req_body maximum request body size, bytes
   * \todo is there a better place to set this up? */
  static void setmaxes(size_t const &max_req_uri, size_t const &max_req_body);
  /** \brief Get a response from an associated client work object.
   * The intended use is the following: a server receives a request from a
   * client that requires the server to act as a client to another server (e.g.
   * proxy or CGI server). While our server is doing this, the original
   * request should lay dormant. When the response comes in from the
   * other server, it should asynchronously wake up the original request
   * and its state, so our server can send the response back to the client on
   * whose behalf we were acting. Thus, this function should be called by
   * the associated client work object.
   * \param assoc associated client work object that holds the respsonse
   * we want to return to the client on whose behalf we were acting
   * \param resphdrs the associated client work object's response headers
   * \param respbody the associated client work object's response body
   * \note The default implementation copies the response headers and body
   * to the server work object; derived classes that override this function may
   * still find it useful to call the base version. Other things that the overriding
   * function may need to do include rescheduling the server work object for
   * a write and deleting the associated client work object. */
  virtual void async_setresponse(HTTP_Client_Work *assoc,
				 structured_hdrs_type const &resphdrs,
				 std::string const &respbody);
protected:
  /** \brief What to do once we have read the request.
   * \param reqhdrs the request headers
   * \param reqbody the request body
   * \param hdrs_dst derived classes should push the response headers to this
   * stream
   * \param body derived classes should point this at the beginning of the
   * response body
   * \param bodysz derived classes should set this to the size of the response
   * body */
  virtual void prepare_response(structured_hdrs_type &reqhdrs,
				std::string const &reqbody,
				std::ostream &hdrs_dst,
				uint8_t const *&body,
				size_t &bodysz);
  /** \brief What to do when we have detected an error and want to let the
   * client know about it.
   * This will be called whenever a derived class throws an HTTP_oops.
   * Thus, it is a general mechanism for escaping the regular "assembly line"
   * and sending back a response right away; occasionally, throwing an HTTP_oops
   * with a status of OK could be useful.
   * \param s HTTP error code we will return
   * \param hdrs_dst derived classes should push the headers here
   * \note The default implementation pushes two headers, the status
   * line and the Server header. */
  virtual void on_oops(HTTP_constants::status const &s, std::ostream &hdrs_dst);
  /** \brief Clear the state of a piece of work without deleting it.
   * This is basically a destructor without the deallocation. It's called
   * at the end of the "assembly line". The reason we just don't delete
   * this piece of work and get a new one later is because HTTP connections
   * are persistent by default; in particular, we can't throw away the input
   * buffer because we might have read the beginning of the next request(s)
   * into it while we were reading in the old request. The default
   * implementation deals with this problem and the default implementation is
   * always called.
   * \todo The need for this function suggests that a "connection", not a "piece
   * of work", is the natural quantum for a server whose connections are
   * persistent. */
  virtual void reset();

  /** In some circumstances, pieces of work need to be able to become dormant,
   * not rescheduling themselves: for example, when we've finished parsing
   * a request, but have to wait for the arrival of some resource before we can
   * send out the response. (Eventually, when we replace blocking disk reads
   * by asynchronous I/O, we can use this too.) */
  bool nosch;

  /** Remember the worker who is working on our behalf, in case we need to
   * extract some state from it. */
  Worker *curworker;

  uint8_t *backup_body; //!< yikes

  static size_t max_req_uri; //!< Maximum Request-URI length to accept
  static size_t max_req_body; //!< Maximum request body size to accept

  // Convenience functions to be called from prepare_response.
  /** \brief Convenience function that can be called from an overriden 
   * prepare_response.
   * \param reqln the request line to parse 
   * \param meth on success, contains the method
   * \param path on success, contains the path parsed from the URI
   * \param query on success, contains the query parsed from the URI
   * \note on failure, throws an HTTP_oops with a status of Bad_Request */
  void parsereqln(std::string const &reqln, HTTP_constants::method &meth,
		  std::string &path, std::string &query);
private:
  // Called by parsereqln
  void parseuri(std::string &uri, std::string &path, std::string &query);
  // Currently unused.
  std::string &uri_hex_escape(std::string &uri);
};

#endif // HTTP_SERVER_WORK_HPP
