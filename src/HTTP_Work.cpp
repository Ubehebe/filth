#include <algorithm>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h" // For PACKAGE_NAME from the build system
#include "HTTP_cmdline.hpp"
#include "HTTP_Origin_Server.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Work.hpp"
#include "logging.h"
#include "ServerErrs.hpp"

using namespace std;
using namespace HTTP_constants;

LockFreeQueue<void *> HTTP_Work::store;
Scheduler *HTTP_Work::sch = NULL;
HTTP_Cache *HTTP_Work::cache = NULL;
Workmap *HTTP_Work::st = NULL;
Time *HTTP_Work::date = NULL;
Compressor *HTTP_Work::compress = NULL;
Magic *HTTP_Work::MIME = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), cl_sz(0), cl_max_fwds(0), stat(OK),
    inhdrs_done(false), outhdrs_done(false)
{
}

HTTP_Work::~HTTP_Work()
{
  cache->unget(path); // fails if path isn't in cache, which is fine
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
    inhdrs_done = true;
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
  try {
    parse_req_line(req.front());
    for (req_type::iterator it = ++req.begin(); it != req.end(); ++it)
      parse_header(*it);
    
    negotiate_content();
  }
  /* Any failure in parsing should cause the worker immediately to prepare a
   * failure message. */
  catch (HTTP_Parse_Err e) {
    stat = e.stat;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Work::parse_req_line(string &line)
{
  _LOG_DEBUG("parse_req_line %d: %s", fd, line.c_str());
  pbuf.clear();
  pbuf.str(line);
  pbuf >> meth;
  string uri;
  pbuf >> uri;
  parse_uri(uri);
  string version;
  pbuf >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
}

// The only function seen by the Worker.
void HTTP_Work::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = (!inhdrs_done) 
      ? rduntil(pbuf, rdbuf, rdbufsz) // Read headers
      : rduntil(pbuf, rdbuf, rdbufsz, cl_sz); // Read body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (!inhdrs_done) {
      if (rdlines()) {
	parse_req();
	// If there's no body to read, schedule a write.
	if (cl_sz == 0)
	  m = write;
      }
    }
    // Finished reading the request body, so schedule a write.
    else if (cl_sz == 0) {
       req_body = pbuf.str();
       m = write;
    }
    sch->reschedule(this);
    break;
  case Work::write:
    err = wruntil(out, outsz);
    if (err == EAGAIN || err == EWOULDBLOCK) {
      sch->reschedule(this);
    }
    // If we're here, we're guaranteed outsz == 0...right?
    else if (err == 0 && outsz == 0) {
      // Done sending headers, now send body.
      if (!outhdrs_done) {
	outhdrs_done = true;
	out = resp_body;
	outsz = resp_body_sz;
	sch->reschedule(this);
      }
      // Done sending body.
      else {
	deleteme = true;
      }
    }
    else {
      throw SocketErr("write", err);
    }
    break;
  }
}

// RFC 2396, sec. 2.4.1. We don't use this yet...
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
  // TODO: throws bad request for proxy-type resources. Support?
  _LOG_DEBUG("parse_uri %s", uri.c_str());
  // Malformed or dangerous URI.
  if (uri[0] != '/' || uri.find("..") != uri.npos)
    throw HTTP_Parse_Err(Bad_Request);

  // Break the URI into a path and a query.
  string::size_type qpos;
  if (uri == "/") {
    path = HTTP_cmdline::c.svals[HTTP_cmdline::default_resource];
  } else if ((qpos = uri.find('?')) != string::npos) {
    path = uri.substr(1, qpos-1);
    query = uri.substr(qpos+1);
  } else {
    path = uri.substr(1);
  }
}

/* After parsing is complete, we talk to the cache and the origin server,
 * if need be, to find the resource. */
void HTTP_Work::negotiate_content()
{
 negotiate_startover:

  HTTP_CacheEntry *c = NULL;
  int err;
  pbuf.str("");
  pbuf.clear();

  // Ask the cache if we have a response ready to go.
  if (cache->get(path, c)) {
    if (c->response_is_fresh() || HTTP_Origin_Server::validate(path, c)) {
      pbuf << *c; // Pushes the status line, then all the headers
      _LOG_DEBUG("%s", pbuf.str().c_str());
      resp_body = c->getbuf();
      resp_body_sz = const_cast<size_t &>(c->sz);
    }
    else {
      cache->advise_evict(path);
      goto negotiate_startover;
    }
  }

  // If not, ask the origin server.
  else if ((err = HTTP_Origin_Server::request(path, c))==0) {
    c->pushstat(stat);
    c->pushhdr(Content_Type, (*MIME)(path.c_str()));
    c->pushhdr(Date, (*date)());
    c->pushhdr(Last_Modified, (*date)(c->last_modified));
    c->pushhdr(Server, PACKAGE_NAME);
    // Might not succeed...
    cache->put(path, c, c->sz);
    pbuf << *c; // Pushes the status line, then all the headers
    _LOG_DEBUG("%s", pbuf.str().c_str());
    resp_body = c->getbuf();
    resp_body_sz = const_cast<size_t &>(c->sz);
  }
  
  else {
    // Make more elaborate?
    switch (err) {
    case ENOENT:
      stat = Not_Found;
      break;
    case EACCES:
      stat = Forbidden;
      break;
    default:
      stat = Internal_Server_Error;
      break;
    }
    pbuf << HTTP_Version << ' ' << stat << CRLF
	 << Date << (*date)() << CRLF
	 << Server << PACKAGE_NAME << CRLF
	 << Content_Length << 0 << CRLF
	 << CRLF;
    resp_body = NULL;
    resp_body_sz = 0;
  }

  // In all three branches, the headers are now flat in pbuf.
  pbuf.get(rdbuf, rdbufsz, '\0');
  out = resp_hdrs = static_cast<char const *>(rdbuf);
  outsz = resp_hdrs_sz = strlen(rdbuf);
  if (outsz == rdbufsz-1)
    _LOG_INFO("headers at least %d bytes long; may have silently truncated",
	      outsz);
}

void HTTP_Work::parse_header(string &line)
{
  _LOG_DEBUG("parse_header %s", line.c_str());
  pbuf.clear();
  pbuf.str(line);
  /* Initialization just to avoid valgrind complaints. All the actual failures
   * should be caught by the exception handling. */
  header h = static_cast<header>(0);
  try {
    pbuf >> h;
  }
  catch (HTTP_Parse_Err e) {
    if (e.stat == Not_Implemented)
      return; // Silently ignore headers we don't implement.
    else
      throw e;
  }
  
  // Comments in this block refer to sections of RFC 2616.
  switch (h) {
  case Accept:
    /* 14.1: "If an Accept header field is present, and if the server cannot
     * send a response which is acceptable according to the combined
     * Accept field value, then the server SHOULD send a 406 (not acceptable)
     * response." Because it says SHOULD, not MUST, we don't need to touch
     * this header. */
    break;
  case Accept_Charset:
    /* 14.2: "If an Accept-Charset header is present, and the server cannot
     * send a response which is acceptable according to the Accept-Charset
     * header, then the server SHOULD send an error response with the 406
     * (not acceptable) status code, though the sending of an unacceptable
     * response is also allowed." We choose the latter. */
    break;
  case Accept_Encoding:
    /* 14.3: "If an Accept-Encoding field is present in a request, and if the
     * server cannot send a response which is acceptable to the Accept-Encoding
     * header, then the server SHOULD send an error response with the 406
     * (Not Acceptable) status code." We choose not to. */
    break;
  case Accept_Language:
    /* 14.4: "If an Accept-Language header is present, then all languages which
     * are assigned a quality factory greater than 0 are acceptable." Note
     * that this does not imply that it is unacceptable to send a resource in
     * a language with a quality factory of 0. Thus, we ignore this header. */
    break;
  case Allow:
    /* 14.7: "The Allow header field MAY be provided with a PUT request to
     * recommend the methods to be supported by the new or modified resource.
     * The server is not required to support these methods and SHOULD include
     * an Allow header in the response giving the actual supported methods." */
    break;
  case Connection:
    /* 14.10: "HTTP/1.1 defines the "close" connection option for the sender to
     * signal that the connection will be closed after completion of the
     * response. */
    if (line.find("close") != line.npos)
      closeme = true;
    break;
  case Content_Encoding:
    /* 14.11: "If the content-encoding of an entity in a request message is not
     * acceptable to the origin server, the server SHOULD respond with a
     * status code of 415 (Unsupported Media Type)." We choose not to. */
    break;
  case Content_Language:
    /* 14.12 says nothing about when this MUST be read. */
    break;
  case Content_Length:
    // 14.13: Content-Length = "Content-Length" ":" 1*DIGIT
    pbuf >> cl_sz;
    break;
  case Content_Location:
    /* 14.13: "The meaning of the Content-Location header in PUT or POST
     * requests is undefined; server are free to ignore it in those cases." */
    break;
  case Content_Type:
    // 14.17: Content-Type = "Content-Type" ":" media-type
    pbuf >> cl_content_type;
    break;
  case Date:
    /* 14.18: "Clients SHOULD only send a Date header field in messages that
     * include an entity-body, as in the case of the PUT and POST requests,
     * and even then it is optional." So we ignore it. */
    break;
  case Expect:
    /* 14.20: Expect = "Expect" ":" 1#expectation
     * expectation = "100-continue" | expectation-extension
     *
     * "A server that does not understand or is unable to comply with any of the
     * expectation values in the Expect field of a request MUST respond with
     * appropriate error status. The server MUST respond with a 417
     * (Expectation Failed) status if any of the expectations cannot be met or,
     * if there are other problems with the request, some other 4xx status."
     * Also: "Comparison of expectation values is case-insensitive for
     * unquoted strings (including the 100-continue token)".
     * 
     * The behavior of 100-continue is governed by 8.2.3. */
    pbuf >> line;
    if (strcasecmp("100-continue", line.c_str())==0)
      throw HTTP_Parse_Err(Continue);
    else if (line.length() > 0)
      throw HTTP_Parse_Err(Expectation_Failed);
    break;
  case Expires:
    /* 14.21. Only useful for proxies? Dunno. */
    pbuf >> cl_expires;
    break;
  case From:
    /* 14.22 */
    pbuf >> cl_from;
    break;
  case Host:
    /* 14.23 */
    pbuf >> cl_host;
    break;
  case Max_Forwards:
    /* 14.31 */
    pbuf >> cl_max_fwds;
    break;
  case Pragma:
    /* 14.32: "All pragma directives specify optional behavior from the
     * viewpoint of the protocol; however, [...] Pragma directives MUST be
     * passed through by a proxy or gateway application, regardless of their
     * significance to that application, since the directives might be applicable
     * to all recipients along the the request/response chain. */
    pbuf >> cl_pragma;
    break;
  case Referer:
    /* 14.36 */
    pbuf >> cl_referer;
    break;
  case Upgrade:
    /* 14.42: "Upgrade cannot be used to insist on a protocol change; its
     * acceptance and use by the server is optional." So we don't do it. */
    break;
  case User_Agent:
    /* 14.43 */
    pbuf >> cl_user_agent;
    break;
  default:
    break;
  }
}

/* RFC 2616 sec. 6: Response =
 * Status-Line *((general-header | response-header | entity-header) CRLF) CRLF
 * [message-body]
 */

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
