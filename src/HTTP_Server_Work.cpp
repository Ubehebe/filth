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
  : HTTP_Work(fd, m)
{
}

HTTP_Server_Work::~HTTP_Server_Work()
{
  cache->unget(path); // fails if path isn't in cache, which is fine
  st->erase(fd);
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
void HTTP_Server_Work::browsehdrs()
{
  if (!reqhdrs[Accept_Encoding].empty()) {
    stringstream tmp(reqhdrs[Accept_Encoding]);
    /* 14.3: "If an Accept-Encoding field is present in a request, and if the
     * server cannot send a response which is acceptable to the Accept-Encoding
     * header, then the server SHOULD send an error response with the 406
     * (Not Acceptable) status code." */
    // TODO strcasecmp  buf >> cl_accept_enc;
  }
  if (!reqhdrs[Connection].empty()) {
    /* 14.10: "HTTP/1.1 defines the "close" connection option for the sender to
     * signal that the connection will be closed after completion of the
     * response. */
    if (reqhdrs[Connection].find("close") != string::npos)
      closeme = true;
  }
  if (!reqhdrs[Expect].empty()) {
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
    if (strcasecmp("100-continue", reqhdrs[Expect].c_str())==0)
      throw HTTP_Parse_Err(Continue);
    else
      throw HTTP_Parse_Err(Expectation_Failed);
  }
  if (!reqhdrs[Max_Forwards].empty()) {
    stringstream tmp(reqhdrs[Max_Forwards]);
    tmp >> cl_max_fwds;
  }
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
