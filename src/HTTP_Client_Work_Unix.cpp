#include "HTTP_Client_Work_Unix.hpp"
#include "HTTP_2616_Server_Work.hpp"
#include "logging.h"

using namespace std;

HTTP_Client_Work_Unix::HTTP_Client_Work_Unix(int fd,
					     HTTP_2616_Server_Work &realclient,
					     structured_hdrs_type const &reqhdrs,
					     string const &req_body)
  : HTTP_Client_Work(fd, reqhdrs, req_body), realclient(realclient)
{
}

void HTTP_Client_Work_Unix::browse_resp(structured_hdrs_type const &resphdrs,
					string const &resp_body)
{
  realclient.async_setresponse(this, resphdrs, resp_body);
}
