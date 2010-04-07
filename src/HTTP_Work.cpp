#include "HTTP_Work.hpp"

Scheduler *HTTP_Work::sch = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), stat(OK), reqhdrs(1+num_header),
    reqhdrs_done(false), resphdrs_done(false), reqbodysz(0)
{
}

void HTTP_Work::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = (reqhdrs_done)
      ? rduntil(parsebuf, cbuf, cbufsz) // Read headers
      : rduntil(parsebuf, cbuf, cbufsz, reqbodysz); // Read body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (!reqhdrs_done) {
      if (parsebuf >> reqhdrs) {
	reqhdrs_done = true;
	if (!reqhdrs[Content_Length].empty()) {
	  parsebuf.clear();
	  parsebuf.str(reqhdrs[Content_Length]);
	  parsebuf >> reqbodysz;
	}
	if (reqbodysz == 0) {
	  browsehdrs();
	  prepare_response();
	  m = write;
	}
      }
    }
    // Finished reading the request body into parsebuf, so schedule a write.
    else if (reqbodysz == 0) {
      reqbody = parsebuf.str();
      prepare_response();
      m = write;
    }
    sch->reschedule(this);
    break;

  case Work::write:
    err = wruntil(out, outsz);
    if (err == EAGAIN || err == EWOULDBLOCK)
      sch->reschedule(this);

    // If we're here, we're guaranteed outsz == 0...right?
    else if (err == 0 && outsz == 0) {
      // Done sending headers, now send body.
      if (!resphdrs_done) {
	resphdrs_done = true;
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

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Work::parsereqln(method &meth, string &path, string &query)
{
  parsebuf.clear();
  parsebuf.str(reqhdrs[reqln]);
  _LOG_DEBUG("parsereqln %s", parsebuf.str().c_str());
  parsebuf >> meth;
  string uri;
  parsebuf >> uri;
  parseuri(uri, path, query);
  string version;
  parsebuf >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
}

void HTTP_Work::parseuri(string &uri, string &path, string &query)
{
  // TODO: throws bad request for proxy-type resources. Support?
  _LOG_DEBUG("rduri %s", uri.c_str());
  // Malformed or dangerous URI.
  if (uri[0] != '/' || uri.find("..") != uri.npos)
    throw HTTP_Parse_Err(Bad_Request);

  // Break the URI into a path and a query.
  string::size_type qpos;
  if (uri == "/") {
    path = "/";
    //    path = HTTP_cmdline::c.svals[HTTP_cmdline::default_resource];
  } else if ((qpos = uri.find('?')) != string::npos) {
    path = uri.substr(1, qpos-1);
    query = uri.substr(qpos+1);
  } else {
    path = uri.substr(1);
  }
}

void HTTP_Work::prepare_response()
{
  parsebuf << HTTP_Version << ' ' << stat << CRLF
	   << Server << PACKAGE_NAME << CRLF
	   << Content_Length << 0 << CRLF
	   << CRLF;
  resp_body = NULL;
  resp_body_sz = 0;

  parsebuf.get(reinterpret_cast<char *>(cbuf), cbufsz, '\0');
  out = resp_hdrs = const_cast<uint8_t *>(cbuf);
  outsz = resp_hdrs_sz = strlen(reinterpret_cast<char *>(cbuf)); // Should this work?????
  if (outsz == cbufsz-1)
    _LOG_INFO("headers at least %d bytes long; may have silently truncated",
	      outsz);
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
