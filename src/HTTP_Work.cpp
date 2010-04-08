#include "HTTP_Work.hpp"
#include "logging.h"

Scheduler *HTTP_Work::sch = NULL;

HTTP_Work::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), stat(OK), req_hdrs(1+num_header),
    req_hdrs_done(false), resp_hdrs_done(false), req_body_sz(0)
{
}

void HTTP_Work::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = (!req_hdrs_done)
      ? rduntil(parsebuf, cbuf, cbufsz) // Read headers
      : rduntil(parsebuf, cbuf, cbufsz, req_body_sz); // Read body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (!req_hdrs_done) {
      if (parsebuf >> req_hdrs) {
	req_hdrs_done = true;
	if (!req_hdrs[Content_Length].empty()) {
	  parsebuf.clear();
	  parsebuf.str(req_hdrs[Content_Length]);
	  parsebuf >> req_body_sz;
	}
	if (req_body_sz == 0)
	  goto dropdown;
      }
    }
    // Finished reading the request body into parsebuf, so schedule a write.
    else if (req_body_sz == 0) {
      req_body = parsebuf.str();
    dropdown:
      parsebuf.str("");
      parsebuf.clear();
      try {
	browse_req(req_hdrs, req_body);
	prepare_response(parsebuf, resp_body, resp_body_sz);
      }
      catch (HTTP_Parse_Err e) {
	parsebuf.str("");
	parsebuf.clear();
	on_parse_err(e.stat, parsebuf, resp_body, resp_body_sz);
      }
      parsebuf.get(reinterpret_cast<char *>(cbuf), cbufsz, '\0');
      out = resp_hdrs = const_cast<uint8_t *>(cbuf);
      outsz = resp_hdrs_sz = strlen(reinterpret_cast<char *>(cbuf)); // Should this work?????
      if (outsz == cbufsz-1)
	_LOG_INFO("headers at least %d bytes long; may have silently truncated",
		  outsz);
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
      if (!resp_hdrs_done) {
	resp_hdrs_done = true;
	out = resp_body;
	outsz = resp_body_sz;
	sch->reschedule(this);
      }
      /* Done sending body. For a persistent connection, instead of deleting,
       * we should reset the state of this object and wait for the socket to
       * become readable again, because the client is pipelining. */
      else if (!closeme) {
	reset();
	HTTP_Work::reset(); // Ensure base reset always called
	m = read;
      }
      // If closeme is true, the worker will delete this piece of work.
    }
    else {
      _LOG_DEBUG();
      throw SocketErr("write", err);
    }
  break;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Work::parsereqln(req_hdrs_type &req_hdrs, method &meth,
			   string &path, string &query)
{
  stringstream tmp(req_hdrs[reqln]);
  _LOG_DEBUG("parsereqln %s", tmp.str().c_str());
  tmp >> meth;
  string uri;
  tmp >> uri;
  parseuri(uri, path, query);
  string version;
  tmp >> version;
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
  } else if ((qpos = uri.find('?')) != string::npos) {
    path = uri.substr(1, qpos-1);
    query = uri.substr(qpos+1);
  } else {
    path = uri.substr(1);
  }
}

void HTTP_Work::prepare_response(stringstream &hdrs, uint8_t const *&body,
				 size_t &bodysz)
{
  hdrs << HTTP_Version << ' ' << stat << CRLF
	   << Server << PACKAGE_NAME << CRLF
	   << Content_Length << 0 << CRLF
	   << CRLF;
  body = NULL;
  bodysz = 0;
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

void HTTP_Work::on_parse_err(status &s, stringstream &hdrs,
			     uint8_t const *&body, size_t &bodysz)
{
  stat = s;
  // This is a minimal message that can't throw anything.
  HTTP_Work::prepare_response(hdrs, body, bodysz); 
}

void HTTP_Work::reset()
{
  req_hdrs_done = resp_hdrs_done = false;
  parsebuf.str("");
  parsebuf.clear();
  cbuf[0] = '\0'; // just paranoia
  req_hdrs.clear();
  req_hdrs.resize(1+num_header);
  req_body.clear();
  req_body_sz = 0;
  resp_hdrs = resp_body = out = NULL;
  resp_hdrs_sz = resp_body_sz = outsz = 0;
}
