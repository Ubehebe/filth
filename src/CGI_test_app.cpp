#include "HTTP_Server_Work.hpp"
#include "Server.hpp"

class CGI_Work : public HTTP_Server_Work
{
private:
  static char const *msg = "hello, world";
public:
  CGI_Work(int fd) : HTTP_Server_Work(fd) {}
  virtual void prepare_response(structured_hdrs_type &reqhdrs,
				std::string const &reqbody,
				std::ostream &hdrs_dst,
				uint8_t const *&body,
				size_t &bodysz)
  {
    hdrs_dst << HTTP_constants::HTTP_Version << ' ' << HTTP_constants::OK << CRLF;
    body = msg;
    bodysz = strlen(msg);
  }
};

class CGI_Server : public Server
{
public:
  CGI_Server(
}

