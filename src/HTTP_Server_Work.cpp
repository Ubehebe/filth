#include <sstream>

#include "HTTP_Server_Work.hpp"
#include "logging.h"

using namespace HTTP_constants;
using namespace std;

HTTP_Server_Work::HTTP_Server_Work(int fd)
  : HTTP_Work(fd, Work::read), nosch(false)
{
}

void HTTP_Server_Work::operator()(Worker *w)
{
 pipeline_continue:
  int err;
  switch(m) {
  case Work::read:
    err = (!inhdrs_done)
      ? rduntil(inbuf, cbuf, cbufsz) // Read headers
      : rduntil(inbody, cbuf, cbufsz, inbody_sz); // Read body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (!inhdrs_done) {
      if (inhdrs_done = (inbuf >> inhdrs)) {
	// We need this to read the rest of the request.
	if (!inhdrs[Content_Length].empty()) {
	  stringstream tmp(inhdrs[Content_Length]);
	  tmp >> inbody_sz;
	}
	if (inbody_sz == 0)
	  goto dropdown;
      }
    }
    // Finished reading the request body into parsebuf, so schedule a write.
    else if (inbody_sz == 0) {
    dropdown:
      try {
	prepare_response(inhdrs, inbody, outbuf, outbody, outbody_sz);
      }
      catch (HTTP_Parse_Err e) {
	outbuf.str("");
	outbuf.clear();
	on_parse_err(e.stat, outbuf, outbody, outbody_sz);
      }
      outbuf.get(reinterpret_cast<char *>(cbuf), cbufsz, '\0');
      out = outhdrs = const_cast<uint8_t *>(cbuf);
      outsz = outhdrs_sz = strlen(reinterpret_cast<char *>(cbuf));
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
      if (!outhdrs_done) {
	outhdrs_done = true;
	out = outbody;
	outsz = outbody_sz;
	sch->reschedule(this);
      }
      else if (nosch) {
	; // Allow this piece of work to become dormant.
      }
      /* Done sending body. For a persistent connection, instead of deleting,
       * we should reset the state of this object and wait for the socket to
       * become readable again, because the client is pipelining. */
      else if (!deleteme) {
	reset();
	HTTP_Server_Work::reset(); // Ensure base reset always called
	m = read;

	/* Try to read an additional test character from the incoming buffer,
	 * in order to determine if there is a pipelined request we need to
	 * preserve for the next worker to handle this piece of work. */
	inbuf.ignore();
	if (inbuf.good()) {
	  inbuf.unget();
	  /* If the client is doing pipelining, just handle the next request
	   * ourselves.
	   *
	   * Q: Couldn't we increase concurrency by having some
	   * mechanism for different workers handling different pipelined
	   * requests from the same client?
	   *
	   * A: Maybe, but this would entail some fundamental design changes;
	   * the "one socket, at most one worker" invariant, so useful for
	   * reasoning about the scheduler, would have to be changed. Besides,
	   * there is an inherent bottleneck here: writes to a socket have to
	   * be sequential, or else the client will get garbage. So I'm just not sure
	   * about the payoff. */
	  goto pipeline_continue;
	}
	else {
	  inbuf.str("");
	  inbuf.clear();
	  sch->reschedule(this);	  
	}
      }
      // If deleteme is true, the worker will delete this piece of work.
    }
    else {
      throw SocketErr("write", err);
    }
  break;
  }
}

// RFC 2616 sec. 5.1: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
void HTTP_Server_Work::parsereqln(string &reqln, method &meth, string &path,
				  string &query)
{
  stringstream tmp(reqln);
  tmp >> meth;
  string uri;
  tmp >> uri;
  parseuri(uri, path, query);
  string version;
  tmp >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
}

void HTTP_Server_Work::parseuri(string &uri, string &path, string &query)
{
  // TODO: throws bad request for proxy-type resources. Support?
  _LOG_DEBUG("rduri %s", uri.c_str());

  // The asterisk is a special URI used with OPTIONS requests (RFC 2616, 9.2)
  if (uri == "*") {
    path = "*";
    return;
  }

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

void HTTP_Server_Work::prepare_response(structured_hdrs_type &reqhdrs,
					string const &reqbody, ostream &hdrstream,
					uint8_t const *&body, size_t &bodysz)
{
  hdrstream << HTTP_Version << ' ' << stat << CRLF
	    << Server << PACKAGE_NAME << CRLF
	    << Content_Length << 0 << CRLF
	    << CRLF;
  body = NULL;
  bodysz = 0;
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

void HTTP_Server_Work::on_parse_err(status &s, ostream &hdrstream,
			     uint8_t const *&body, size_t &bodysz)
{
  stat = s;
  structured_hdrs_type fake1;
  string fake2;
  // This is a minimal message that can't throw anything.
  HTTP_Server_Work::prepare_response(fake1, fake2, hdrstream, body, bodysz); 
}

void HTTP_Server_Work::reset()
{
  // TODO: put in base class?
  inhdrs_done = outhdrs_done = false;
  // Don't clear inbuf!
  outbuf.str("");
  outbuf.clear();
  cbuf[0] = '\0';
  inhdrs.clear();
  inhdrs.resize(1+num_header);
  inbody.clear();
  inbody_sz = 0;
  outhdrs = outbody = out = NULL;
  outhdrs_sz = outbody_sz = outsz = 0;
}
