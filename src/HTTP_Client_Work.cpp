#include <string.h>

#include "HTTP_Client_Work.hpp"
#include "HTTP_constants.hpp"
#include "HTTP_parsing.hpp"
#include "ServerErrs.hpp"
#include "logging.h"

using namespace std;
using namespace HTTP_constants;

HTTP_Client_Work::HTTP_Client_Work(int fd, structured_hdrs_type &reqhdrs,
				   string const &req_body)
  : HTTP_Work(fd, Work::write)
{
  stringstream tmp;
  tmp << reqhdrs << CRLF;
  tmp.get(reinterpret_cast<char *>(cbuf), cbufsz, '\0');
  out = outhdrs = const_cast<uint8_t *>(cbuf);
  outsz = outhdrs_sz = strlen(reinterpret_cast<char *>(cbuf));
  if (outsz == cbufsz-1) {
    _LOG_INFO("headers at least %d bytes long; may have silently truncated",
	      outsz);
  }
  if ((outbody_sz = req_body.length()) > 0) {
    try {
      outbody = new uint8_t[outbody_sz];
      uint8_t *tmp = const_cast<uint8_t *>(outbody);
      strncpy(reinterpret_cast<char *>(tmp), req_body.c_str(), outbody_sz);
    } catch (bad_alloc) {
      _LOG_INFO("couldn't allocate %d bytes for request body, truncating",
		outbody_sz);
      outbody = NULL;
      outbody_sz = 0;
    }
  }
}

HTTP_Client_Work::~HTTP_Client_Work()
{
  delete[] outbody;
}

void HTTP_Client_Work::operator()()
{
  int err;
  switch (m) {
  case Work::write:
    err = wruntil(out, outsz);
    if (err == EAGAIN || err == EWOULDBLOCK)
      sch->reschedule(this);
    else if (err == 0 && outsz == 0) {
      // Finished writing request headers.
      if (!outhdrs_done) {
	outhdrs_done = true;
	out = outbody;
	outsz = outbody_sz;
      }
      // Finished writing request body, prepare for a read.
      else {
	m = Work::read;
      }
      sch->reschedule(this);
    }
    else {
      throw SocketErr("write", err);
    }
    break;
  case Work::read:
    err = (!inhdrs_done)
      ? rduntil(inbuf, cbuf, cbufsz) // read response headers
      : rduntil(inbody, cbuf, cbufsz, inbody_sz); // read response body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    // Try to parse response headers 
    else if (!inhdrs_done) {
      if (inhdrs_done = (inbuf >> inhdrs)) {
	if (!inhdrs[Content_Length].empty()) {
	  stringstream tmp(inhdrs[Content_Length]);
	  tmp >> inbody_sz;
	}
	if (inbody_sz == 0)
	  goto dropdown;
      }
      sch->reschedule(this);
    }
    // Finished parsing response headers and reading in response body.
    else if (inbody_sz == 0) {
    dropdown:
      browse_resp(inhdrs, inbody);
    }
    else {
      sch->reschedule(this);
    }
    break;
  }
}

void HTTP_Client_Work::browse_resp(structured_hdrs_type &resphdrs,
				   string &resp_body)
{
  deleteme = true;
}
