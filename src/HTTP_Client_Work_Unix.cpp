#include "HTTP_Client_Work_Unix.hpp"

using namespace std;

HTTP_Client_Work_Unix::HTTP_Client_Work_Unix(int fd, int assocfd,
					     structured_hdrs_type &reqhdrs,
					     string const &req_body)
  : HTTP_Client_Work(fd, reqhdrs, req_body), assocfd(assocfd)
{
}
