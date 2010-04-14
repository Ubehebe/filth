#ifndef HTTP_CLIENT_WORK_UNIX_HPP
#define HTTP_CLIENT_WORK_UNIX_HPP

#include "HTTP_Client_Work.hpp"

class HTTP_2616_Server_Work; // fwd declaration for ref below

/* This is a class for a special kind of HTTP client: an HTTP
 * server, acting as a client to a Unix domain socket to get a resource
 * on behalf of the "real" client. The intended behavior is as follows:
 * the HTTP server realizes the resource requested by the client (in an
 * HTTP_Server_Work object) is a Unix domain socket. The server initiates
 * a new connection to the domain socket, creates an instance of
 * HTTP_Client_Work_Unix representing that connection, and schedules it for
 * a write. The server does _not_ reschedule the HTTP_Server_Work object at
 * that time. Meanwhile, the HTTP_Client_Work_Unix object does its writing
 * and reading. After it is done browsing the response, it wakes up the dormant
 * HTTP_Server_Work object via its assocfd. This object is then able to proceed
 * with its response back to the original client.
 *
 * Note that the connection with the original client stays alive even though
 * no reads/writes are scheduled; this is true even if we are using TCP
 * keepalives.
 *
 * There is a possible race condition: the HTTP_Client_Work_Unix object
 * could finish its work before the associated HTTP_Server_Work object
 * even goes dormant. To avoid this I think we need to make sure that
 * scheduling the HTTP_Client_Work_Unix object is the last thing the
 * HTTP_Server_Work object does before going dormant. */
class HTTP_Client_Work_Unix : public HTTP_Client_Work
{
public:
  HTTP_Client_Work_Unix(int fd, HTTP_2616_Server_Work &realclient,
			structured_hdrs_type &reqhdrs,
			std::string const &req_body);
  ~HTTP_Client_Work_Unix() {}
  void browse_resp(structured_hdrs_type const &resphdrs,
		   std::string const &resp_body);
private:
  HTTP_2616_Server_Work &realclient;
};

#endif // HTTP_CLIENT_WORK_UNIX_HPP
