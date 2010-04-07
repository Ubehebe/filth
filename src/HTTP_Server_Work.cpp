#include <algorithm>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>

#include "compression.hpp"
#include "config.h" // For PACKAGE_NAME
#include "HTTP_cmdline.hpp"
#include "HTTP_Origin_Server.hpp"
#include "HTTP_Parse_Err.hpp"
#include "HTTP_Server_Work.hpp"
#include "logging.h"
#include "ServerErrs.hpp"

using namespace std;
using namespace HTTP_constants;

LockFreeQueue<void *> HTTP_Server_Work::store;
HTTP_Cache *HTTP_Server_Work::cache = NULL;
Workmap *HTTP_Server_Work::st = NULL;
Time *HTTP_Server_Work::date = NULL;
Magic *HTTP_Server_Work::MIME = NULL;

HTTP_Server_Work::HTTP_Server_Work(int fd, Work::mode m)
  : HTTP_Work<HTTP_Server_Work>(fd, m)
{
}

HTTP_Server_Work::~HTTP_Server_Work()
{
  cache->unget(path); // fails if path isn't in cache, which is fine
  st->erase(fd);
}

// RFC 2396, sec. 2.4.1. We don't use this yet...
string &HTTP_Server_Work::uri_hex_escape(string &uri)
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

/* After parsing is complete, we talk to the cache and the origin server,
 * if need be, to find the resource. */
void HTTP_Server_Work::prepare_response()
{
  /*
 prepare_startover:

  HTTP_CacheEntry *c = NULL;
  int err;
  parsebuf.str("");
  pbuf.clear();

  // Ask the cache if we have a response ready to go.
  if (cache->get(path, c)) {
    if (c->response_is_fresh() || HTTP_Origin_Server::validate(path, c)) {
      pbuf << *c; // Pushes the status line, then all the headers
      _LOG_DEBUG("%s", pbuf.str().c_str());
      resp_body = c->getbuf();
      resp_body_sz = const_cast<size_t &>(c->szincache);
    }
    else {
      cache->advise_evict(path);
      goto prepare_startover;
    }
  }

  // If not, ask the origin server.
  else if ((err = HTTP_Origin_Server::request(path, c))==0) {
    /* Any header that requires thread-safe state to construct (like
     * the MIME lookup) gets pushed here.
    c->pushstat(stat);
    c->pushhdr(Content_Type, (*MIME)(path.c_str()));
    c->pushhdr(Date, (*date)());
    c->pushhdr(Last_Modified, (*date)(c->last_modified));
    // Might not succeed...
    cache->put(path, c, c->szincache);
    pbuf << *c; // Pushes the status line, then all the headers
    _LOG_DEBUG("%s", pbuf.str().c_str());
    resp_body = c->getbuf();
    resp_body_sz = const_cast<size_t &>(c->szincache);
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
  pbuf.get(reinterpret_cast<char *>(rdbuf), rdbufsz, '\0');
  out = resp_hdrs = const_cast<uint8_t *>(rdbuf);
  outsz = resp_hdrs_sz = strlen(reinterpret_cast<char *>(rdbuf)); // Should this work?????
  if (outsz == rdbufsz-1)
    _LOG_INFO("headers at least %d bytes long; may have silently truncated",
	      outsz);
*/
}

// Comments in this block refer to sections of RFC 2616.
void HTTP_Server_Work::Accept_Encoding(stringstream &buf)
{
  /* 14.3: "If an Accept-Encoding field is present in a request, and if the
   * server cannot send a response which is acceptable to the Accept-Encoding
   * header, then the server SHOULD send an error response with the 406
   * (Not Acceptable) status code." */
  // TODO strcasecmp  buf >> cl_accept_enc;    
}

void HTTP_Server_Work::Connection(stringstream &buf)
{
  /* 14.10: "HTTP/1.1 defines the "close" connection option for the sender to
   * signal that the connection will be closed after completion of the
   * response. */
  if (buf.str().find("close") != string::npos)
    closeme = true;
}
void HTTP_Server_Work::Content_Length(std::stringstream &buf)
{
  // 14.13: Content-Length = "Content-Length" ":" 1*DIGIT
  buf >> reqbodysz; // Protected member of the base class
}

void HTTP_Server_Work::Content_Type(std::stringstream &buf)
{
  // 14.17: Content-Type = "Content-Type" ":" media-type
  getline(buf, cl_content_type);
}

void HTTP_Server_Work::Expect(stringstream &buf)
{
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
  string line;
  getline(buf, line);
  if (strcasecmp("100-continue", line.c_str())==0)
      throw HTTP_Parse_Err(Continue);
    else if (line.length() > 0)
      throw HTTP_Parse_Err(Expectation_Failed);
}

void HTTP_Server_Work::Expires(stringstream &buf)
{
  // 14.21. Only useful for proxies? Dunno.
  getline(buf, cl_expires);
}

void HTTP_Server_Work::From(stringstream &buf)
{
  // 14.22
  getline(buf, cl_from);
}

void HTTP_Server_Work::Host(stringstream &buf)
{
  // 14.23
  getline(buf, cl_host);
}

void HTTP_Server_Work::Max_Forwards(stringstream &buf)
{
  // 14.31
  buf >> cl_max_fwds;
}

void HTTP_Server_Work::Pragma(stringstream &buf)
{
  /* 14.32: "All pragma directives specify optional behavior from the
   * viewpoint of the protocol; however, [...] Pragma directives MUST be
   * passed through by a proxy or gateway application, regardless of their
   * significance to that application, since the directives might be applicable
   * to all recipients along the the request/response chain. */
  getline(buf, cl_pragma);
}

void HTTP_Server_Work::Referer(stringstream &buf)
{
  getline(buf, cl_referer);
}

void HTTP_Server_Work::User_Agent(stringstream &buf)
{
  // 14.43
  getline(buf, cl_user_agent);
}

void *HTTP_Server_Work::operator new(size_t sz)
{
  void *stuff;
  if (!store.nowait_deq(stuff)) {
    stuff = ::operator new(sz);
  }
  return stuff;
}

void HTTP_Server_Work::operator delete(void *work)
{
  store.enq(work);
}
