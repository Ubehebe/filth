#include <iostream>

#include "HTTP_constants.hpp"
#include "HTTP_Server_Work.hpp"
#include "Preallocated.hpp"
#include "Server.hpp"
#include "Work.hpp"
#include "Worker.hpp"

class CGI_Work : public HTTP_Server_Work, public Preallocated<CGI_Work>
{
private:
  static char const *msg;
public:
  CGI_Work(int fd, Work::mode m=Work::read) : HTTP_Server_Work(fd) {}
  ~CGI_Work() { curworker->fwork->unregister(fd); }
  virtual void prepare_response(structured_hdrs_type &reqhdrs,
				std::string const &reqbody,
				std::ostream &hdrs_dst,
				uint8_t const *&body,
				size_t &bodysz)
  {
    body = reinterpret_cast<uint8_t const *>(msg);
  bodysz = strlen(msg);
  HTTP_constants::status s = HTTP_constants::OK;
  hdrs_dst << HTTP_constants::HTTP_Version << ' ' << s << HTTP_constants::CRLF
  << HTTP_constants::Content_Length << bodysz << HTTP_constants::CRLF
  << HTTP_constants::CRLF;
}

  };

char const *CGI_Work::msg = "hello, world";

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <name>\n";
    exit(1);
  }
  openlog((argv[0][0] == '.' && argv[0][1] == '/') ? &argv[0][2] : argv[0],
	  SYSLOG_OPTS, LOG_USER);
  Server<CGI_Work, Worker>(AF_LOCAL, ".", argv[1]).serve();
}

