#include <algorithm>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
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

LockFreeQueue<void *> HTTP_Work::store;
Scheduler *HTTP_Work::sch = NULL;
FileCache *HTTP_Work::cache = NULL;
Workmap *HTTP_Work::st = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), status_line_done(false), resource(NULL)
{
}

HTTP_Work::~HTTP_Work()
{
  // If we're using the cache, tell it we're done.
  if (resource != NULL && stat == OK)
    cache->release(path);
  st->erase(fd);
}

// Returns true if we've finished chunking the request into lines, false else.
bool HTTP_Work::rdlines()
{
  string line;

  /* RFC 2616 sec. 5: Request = Request-Line
   * *((general-header | request-header | entity-header) CRLF)
   * CRLF [message-body] */

  /* Get a line until there are no more lines, or we hit the CRLF line.
   * Thus we're ignoring the message body for now. */
  while (getline(pbuf, line, '\r') && line.length() > 0) {
    _LOG_DEBUG("parse: line %s", line.c_str());
    /* If the line isn't properly terminated, save it and report that we
     * need more text from the client in order to parse. */
    if (pbuf.peek() != '\n') {
      pbuf.clear();
      pbuf.str(line);
      return false;
    }
    pbuf.ignore(); // Ignore the \n (the \r is already consumed)
    req.push_back(line);
  }
  pbuf.clear();
  // We got to the empty (CRLF) line.
  if (line.length() == 0 && pbuf.peek() == '\n') {
    stat = OK;
    return true;
  } else {
    _LOG_DEBUG("line not properly terminated: %s", line.c_str());
    pbuf.str(line);
    return false;
  }
}

/* RFC 2616 sec. 5: Request = Request-Line
 * *((general-header | request-header | entity-header) CRLF)
 * CRLF [message-body] */
void HTTP_Work::parse_req()
{
  // Any failure in parsing should stop the worker immediately.
  try {
    parse_req_line(req.front());
    for (list<string>::iterator it = ++req.begin(); it != req.end(); ++it)
      parse_header(*it);
  } catch (HTTP_Parse_Err e) {
    stat = e.stat;
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
}

// The only function seen by the Worker.
void HTTP_Work::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = rduntil(pbuf, rdbuf, rdbufsz);
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (rdlines()) {
      parse_req();
      format_status_line();
      m = write;
    }
    sch->reschedule(this);
    break;
  case Work::write:
    err = wruntil(outgoing_offset, outgoing_offset_sz);
    if (err == EAGAIN || err == EWOULDBLOCK) {
      sch->reschedule(this);
    }
    // If we're here, we're guaranteed outgoing_offset_sz == 0...right?
    else if (err == 0) {
      if (!status_line_done) {
	status_line_done = true;
	outgoing_offset = resource;
	outgoing_offset_sz = resourcesz;
	sch->reschedule(this);
      } else {
	deleteme = true;
      }
    }
    else {
      throw SocketErr("write", err);
    }
    break;
  }
}

// RFC 2396, sec. 2.4.1.
string &HTTP_Work::uri_hex_escape(string &uri)
{
  size_t start = 0;
  stringstream hexbuf;
  int c;
  while ((start = uri.find('%', start)) != uri.npos) {
    // Every % needs two additional chars.
    if (start + 2 >= uri.length())
      throw HTTP_Parse_Err(Bad_Request);
    hexbuf << uri[start+1] << uri[start+2];
    hexbuf >> hex >> c;
    uri.replace(start, 3, 1, (char) c);
  }
  return uri;
}

void HTTP_Work::parse_uri(string &uri)
{
  _LOG_DEBUG("parse_uri %s", uri.c_str());
  // Malformed or dangerous URI.
  if (uri[0] != '/' || uri.find("..") != uri.npos)
    throw HTTP_Parse_Err(Bad_Request);

  // Break the URI into a path and a query.
  string::size_type qpos;
  if (uri == "/") {
    path = HTTP_cmdline::c.svals[HTTP_cmdline::default_resource];
  }
  else if ((qpos = uri.find('?')) != string::npos) {
    path = uri.substr(1, qpos-1);
    query = uri.substr(qpos+1);
  }
  else {
    path = uri.substr(1);
  }

 parse_uri_tryagain:
  int err = cache->reserve(path, resource, resourcesz);

  switch (err) {
  case 0: break;
  case EACCES:
    throw HTTP_Parse_Err(Forbidden);
  case EINVAL:
    // In cache but invalidated; try again soon.
    sleep(1);
    goto parse_uri_tryagain;
  case EISDIR:
    throw HTTP_Parse_Err(Not_Implemented);
  case ENOENT:
    throw HTTP_Parse_Err(Not_Found);
  case ENOMEM:
    throw HTTP_Parse_Err(Internal_Server_Error);
  case ESPIPE:
    // This is where to add support for dynamic resources.
    throw HTTP_Parse_Err(Not_Implemented);
  default:
    throw HTTP_Parse_Err(Internal_Server_Error);
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
}

/* RFC 2616 sec. 6.1:
 * Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF */
inline void HTTP_Work::format_status_line()
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

void *HTTP_Work::operator new(size_t sz)
{
  void *stuff;
  if (!store.nowait_deq(stuff)) {
    stuff = ::operator new(sz);
  }
  return stuff;
}

void HTTP_Work::operator delete(void *work)
{
  store.enq(work);
}
