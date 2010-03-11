#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>

#include "HTTP_cmdline.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Work.hpp"
#include "ServerErrs.hpp"

using namespace std;
using namespace HTTP_constants;

LockedQueue<Work *> *HTTP_Work::q = NULL;
Scheduler *HTTP_Work::sch = NULL;
FileCache *HTTP_Work::cache = NULL;
unordered_map<int, Work *> *HTTP_Work::st = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), fake(false), erasemyself(true), req_line_done(false),
    status_line_done(false), resource(NULL)
{
  memset((void *)&rdbuf, 0, rdbufsz);
}

HTTP_Work::~HTTP_Work()
{
  if (!fake) {
    // If we're using the cache, tell it we're done.
    if (resource != NULL)
      cache->release(path);
    // Bye client!
    close(fd);
    // Remove yourself from the state map. TODO
    if (erasemyself)
      st->erase(fd);
  }
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
      stat = OK;
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
  /* TODO: right now we schedule a write immediately after the reading is
   * complete. But when we add more asynchronous/nonblocking I/O on
   * the server side (e.g. wait for a CGI program to return), we would want
   * the write to be scheduled only when all the resources are ready. */
  if (parse()) {
    format_status_line();
    m = write;
  }
  sch->reschedule(this);
}

void HTTP_Work::outgoing()
{
  if (!status_line_done) {
    outgoing(reinterpret_cast<char **>(&rdbuf), &statlnsz);
    if (statlnsz == 0)
      status_line_done = true;
  }
  else {
    outgoing(&resource, &resourcesz);
    if (resourcesz == 0)
      deleteme = true;
  }
}

void HTTP_Work::outgoing(char **buf, size_t *towrite)
{
  ssize_t nwritten;
  while (true) {
    if ((nwritten = ::write(fd, (void *) *buf, *towrite))>0) {
      *buf += nwritten;
      *towrite -= nwritten;
    }
    else if (nwritten == -1 && errno == EINTR)
      continue;
    else
      break;
  }
  if (nwritten == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      sch->reschedule(this);
    else
      throw SocketErr("write", errno);
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

  // Check the cache.
  if ((resource = cache->reserve(path, resourcesz))==NULL) {
    ; // TODO
  }
  // Did find it in the cache--remember how big it is.
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
  // Note that we reuse the read buffer, which ought not to be in use now...
  snprintf(rdbuf, rdbufsz, "%s %d %s\r\n",
	   HTTP_Version,
	   status_vals[stat],
	   status_strs[stat]);
  statlnsz = strlen(rdbuf);
}

void HTTP_Work::init(LockedQueue<Work *> *_q, Scheduler *_sch,
		     FileCache *_cache, unordered_map<int, Work *> *_st)
{
  q = _q;
  sch = _sch;
  cache = _cache;
  st = _st;
}

Work *HTTP_Work::getwork(int fd, Work::mode m)
{
  /* TODO: think about synchronization.
   * Note that if fd is found in the state map, the second parameter
   * is ignored. Is this the right thing to do? */
  unordered_map<int, Work *>::iterator it;
  if ((it = st->find(fd)) != st->end())
    return it->second;
  else {
    HTTP_Work *w = new HTTP_Work(fd, m);
    (*st)[fd] = w;
    return w;
  }
}
