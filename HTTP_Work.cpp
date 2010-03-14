#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>

#include "HTTP_cmdline.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Work.hpp"
#include "logging.h"
#include "ServerErrs.hpp"

using namespace std;
using namespace HTTP_constants;

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;
FileCache *HTTP_Work::cache = NULL;
HTTP_Statemap *HTTP_Work::st = NULL;

void HTTP_Work::static_init(LockedQueue<Work *> *_q, Scheduler *_sch,
			    FileCache *_cache, HTTP_Statemap *_st)
{
  HTTP_Work::q = _q;
  HTTP_Work::sch = _sch;
  HTTP_Work::cache = _cache;
  HTTP_Work::st = _st;
}

// Dummy constructor.
HTTP_Work::HTTP_Work()
  : Work(-1, Work::read), resource(NULL)
{
}

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), req_line_done(false),
    status_line_done(false), resource(NULL)
{
  memset((void *)&rdbuf, 0, rdbufsz);
}

HTTP_Work::~HTTP_Work()
{
  /* Objects allocated through the dummy constructor have fd==-1.*/
  if (fd >= 0) {
    _LOG_DEBUG("close %d", fd);
    close(fd);
  }
  // If we're using the cache, tell it we're done.
  if (resource != NULL && stat == OK)
    cache->release(path);
  // If fd==-1, this will fail but that's OK.
  st->erase(fd);
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
	_LOG_DEBUG("parse %d: line not properly terminated: %s",
		   fd, line.c_str());
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
      _LOG_DEBUG("parse %d: complete", fd);
      pbuf.str("");
      stat = OK;
      return true;
    }
    else {
      _LOG_DEBUG("parse %d: line not properly terminated: %s",
		 fd, line.c_str());
      pbuf.str(line);
      return false;
    }
  }
  // Any failure in parsing should stop the worker immediately.
  catch (HTTP_Parse_Err e) {
    stat = e.stat;
    return true;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Work::parse_req_line(string &line)
{
  _LOG_DEBUG("parse_req_line %d: %s", fd, line.c_str());
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
  case Work::read: incoming(); break;
  case Work::write: outgoing(); break;
  }
}

void HTTP_Work::incoming()
{
  ssize_t nread;
  /* Read until we would block.
   * My understanding is reading a socket will return 0
   * iff the peer hangs up. However, the scheduler already
   * checks the file descriptor for hangups, which is why
   * we don't check for nread==0 here. ??? */
  while (true) {
    if ((nread = ::read(fd, (void *)rdbuf, rdbufsz-1))>0) {
      rdbuf[nread] = '\0';
      _LOG_DEBUG("read %d: %s", fd, rdbuf);
      pbuf << rdbuf;
    }
    /* Interrupted by a system call. I'm currently blocking signals to
     * workers, but just in case I decide to change that. */
    else if (nread == -1 && errno == EINTR) {
      _LOG_DEBUG("read %d: %m, continuing", fd);
      continue;
    }
    else
      break;
  }
  if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    throw SocketErr("read", errno);
  /* TODO: right now we schedule a write immediately after the reading is
   * complete. But when we add more asynchronous/nonblocking I/O on
   * the server side (e.g. wait for a CGI program to return), we would want
   * the write to be scheduled only when all the resources are ready. */
  if (parse()) {
    format_status_line();
    m = write;
  }
  _LOG_DEBUG("rescheduling %d for %s", fd, (m == read) ? "read" : "write");
  sch->reschedule(this);
}

void HTTP_Work::outgoing()
{
  if (!status_line_done) {
    outgoing(statlnsz);
    if (statlnsz == 0) {
      status_line_done = true;
      outgoing_offset = resource;
      sch->reschedule(this);
    }
  }
  else {
    outgoing(resourcesz);
    if (resourcesz == 0)
      deleteme = true;
  }
}

void HTTP_Work::outgoing(size_t &towrite)
{
  ssize_t nwritten;
  char save;
  while (true) {
    if ((nwritten = ::write(fd, (void *) outgoing_offset, towrite))>0) {
      save = *(outgoing_offset+nwritten);
      *(outgoing_offset+nwritten) = '\0';
      _LOG_DEBUG("write %d: %s", fd, outgoing_offset);
      *(outgoing_offset+nwritten) = save;
      outgoing_offset += nwritten;
      towrite -= nwritten;
    }
    else if (nwritten == -1 && errno == EINTR) {
      _LOG_DEBUG("write %d: %m, continuing", fd);
      continue;
    }
    else
      break;
  }
  if (nwritten == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      _LOG_DEBUG("outgoing %d: rescheduling write", fd);
      sch->reschedule(this);
    }
    else {
      _LOG_DEBUG("outgoing %d: write: %m", fd);
      throw SocketErr("write", errno);
    }
  }
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

  // Check the cache; for now we don't support dynamic resources
  if ((resource = cache->reserve(path, resourcesz)) == NULL)
    throw HTTP_Parse_Err(Not_Found);

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

/* RFC 2616 sec. 6.1:
 * Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF */
void HTTP_Work::format_status_line()
{
  /* For now we put the CRLF separating the response headers from the
   * response message body on the end of the status line. Will need to
   * change once we reply with additional response headers...*/
  // Note that we reuse the read buffer, which ought not to be in use now...
  snprintf(rdbuf, rdbufsz, "%s %d %s\r\n\r\n",
	   HTTP_Version,
	   status_vals[stat],
	   status_strs[stat]);
  char *tmp = rdbuf;
  // Dirty trick...
  while ((tmp = strchr(tmp, '_'))!=NULL)
    *tmp = ' ';
  _LOG_DEBUG("format_status_line %d: %s", fd, rdbuf);
  statlnsz = strlen(rdbuf);
  outgoing_offset = rdbuf;
  // For now, just copy any error response to the message body itself.
  if (stat != OK) {
    resource = rdbuf;
    resourcesz = statlnsz;
  }
}

Work *HTTP_Work::getwork(int fd, Work::mode m)
{
  /* TODO: think about synchronization.
   * Note that if fd is found in the state map, the second parameter
   * is ignored. Is this the right thing to do? */
  HTTP_Statemap::iterator it;
  if ((it = st->find(fd)) != st->end())
    return it->second;
  else {
    HTTP_Work *w = new HTTP_Work(fd, m);
    (*st)[fd] = w;
    return w;
  }
}
