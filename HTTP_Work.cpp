#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>

#include "HTTP_cmdline.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Work.hpp"
#include "ServerErrs.hpp"

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;
FileCache *HTTP_Work::cache = NULL;

using namespace std;
using namespace HTTP_constants;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), req_line_done(false)
{
  memset((void *)&rdbuf, 0, rdbufsz);
}

// Returns true if we don't need to parse anything more, false otherwise.
bool HTTP_Work::parse()
{
  string line;

  /* RFC 2616 sec. 5: Request = Request-Line
   * *((general-header | request-header | entity-header) CRLF)
   * CRLF [message-body] */

  try {
    /* Get a line until there are no more lines, or we hit the CRLF line.
     * Thus we're ignoring the message body for now. */
    while (getline(pbuf, line, '\r') && line.length() > 1) {
      /* If the line isn't properly terminated, save it and report that we
       * need more text from the client in order to parse. */
      if (pbuf.peek() != '\n') {
	pbuf.clear();
	pbuf.str(line);
	return false;
      }
      pbuf.ignore(); // Ignore the \n (the \r is already consumed)
      (req_line_done) ? parse_header(line) : parse_req_line(line);
    }
    pbuf.clear();
    // We got to the empty (CRLF) line.
    if (line.length() == 1 && pbuf.peek() == '\n') {
      pbuf.str("");
      return true;
    }
    else {
      pbuf.str(line);
      return false;
    }
  }
  // Any failure in parsing should stop the worker immediately.
  catch (HTTP_Parse_Err e) {
    stat = e.stat;
    pbuf.clear();
    pbuf.str("");
    return true;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Work::parse_req_line(string &line)
{
  istringstream tmp(line);
  tmp >> meth;
  string uri;
  tmp >> uri;
  parse_uri(uri);
  string version;
  tmp >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
  req_line_done = true;
}

void HTTP_Work::operator()()
{
  switch(m) {
  case Work::read: get_from_client(); break;
  case Work::write: put_to_client(); break;
  }
}

void HTTP_Work::get_from_client()
{
  ssize_t nread;
 /* Read until we would block.
  * My understanding is reading a socket will return 0
  * iff the peer hangs up. However, the scheduler already
  * checks the file descriptor for hangups, which is why
  * we don't check for nread==0 here. ??? */
  while (true) {
    if ((nread = ::read(fd, (void *)rdbuf, rdbufsz))>0)
      pbuf << rdbuf;
    /* Interrupted by a system call. I'm currently blocking signals to
     * workers, but just in case I decide to change that. */
    else if (nread == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    throw SocketErr("read", errno);
  if (!parse())
    sch->reschedule(this);
  else
    ; // schedule write? or wait for asynchronous I/O to complete?
}

void HTTP_Work::put_to_client()
{

}

void HTTP_Work::parse_uri(string &uri)
{
  // Malformed or dangerous URI.
  if (uri[0] != '/' || uri.find("..") != string::npos)
    throw HTTP_Parse_Err(Bad_Request);

  // Break the URI into a path and a query.
  string::size_type qpos;
  if (uri == "/") {
    path = HTTP_cmdline::svals[HTTP_cmdline::default_resource];
  }
  else if ((qpos = uri.find('?')) != string::npos) {
    path = uri.substr(1, qpos-1);
    query = uri.substr(qpos+1);
  }
  else {
    path = uri.substr(1);
  }

  struct stat statbuf;

  // Check the cache.
  if ((resource = cache->reserve(path, &statbuf))==NULL) {
    if (S_ISSOCK(statbuf.st_mode)) {
      ; // dynamic resource...do something!
    }
    else throw HTTP_Parse_Err(Not_Found);
  }
}

void HTTP_Work::parse_header(string &line)
{
  istringstream tmp(line);
  header h;
  try {
    tmp >> h;
  }
  // For now, silently ignore headers we don't implement.
  catch (HTTP_Parse_Err e) {
    if (e.stat != Not_Implemented)
      throw e;
  }
  // A huge list.
  switch (h) {
  default: break;
  }
}

void HTTP_mkWork::init(LockedQueue<Work *> *q, Scheduler *sch, FileCache *cache)
{
  HTTP_Work::q = q;
  HTTP_Work::sch = sch;
  HTTP_Work::cache = cache;
}

Work *HTTP_mkWork::operator()(int fd, Work::mode m)
{
  return new HTTP_Work(fd, m);
}
