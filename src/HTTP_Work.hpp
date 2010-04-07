#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <config.h>
#include <sstream>
#include <string>
#include <string.h>

#include "HTTP_constants.hpp"
#include "HTTP_Parse_Err.hpp"
#include "Scheduler.hpp"
#include "Work.hpp"

using namespace std;
using namespace HTTP_constants;

/* A general class for any "unit of work" that reads an HTTP request
 * and writes an HTTP response. 
 *
 * NOTE
 * Some resources should be shared among all such "units of work", for example
 * a pointer to the scheduler in case we cannot complete the unit of work
 * in one go and need to reschedule for later. Unfortunately, there is no
 * easy way in C++ to express the pattern "all the instances of each derived
 * class should share resource X". "All the instances of each derived class
 * should ..." sounds like "virtual", and "share resource X" sounds like
 * "static", but someting cannot be both virtual and static. (Member data
 * in particular cannot be virtual at all.)
 *
 * I experimented with a workaround: instead of inheritance, use non-class
 * template parameters for the data that ought to be shared. I think this is
 * too advanced for me at this point; I was getting syntax errors I didn't
 * really understand, such as the template parameters having to have external
 * linkage. 
 *
 * This creates one big drawback: every instance of EVERY CLASS that derives
 * from HTTP_Work shares HTTP_Work's static data. What this means is that
 * in any application you should have at most one class deriving HTTP_Work.
 * I think this is reasonable if not ideal. Two entities parsing HTTP in two
 * different ways should be two different processes; there's no reason
 * to communicate using HTTP within a single process.
 */
template<class Derived> class HTTP_Work : public Work
{
public:
  void operator()();
  // The main function Derived should override.
  virtual void prepare_response();
  static void setsch(Scheduler *_sch);
  HTTP_Work(int fd, Work::mode m);

protected:
  /* This base class does as little parsing as possible, but there are some
   * things that always need to be parsed, like the request line and the
   * Content-Length header. Allow derived classes to access them. */
  size_t reqbodysz;
  method meth;
  string path, query;

private:
  /* Tracks progress through the request->response assembly line.
   * There's a req1st_done because the first line of a request requires
   * special parsing, but there's no resp1st_done because the first line
   * of a response is just stored and flattened along with all the other
   * response headers. */
  enum { nothing_done, req1st_done, reqhdrs_done, resphdrs_done } progress;

  void rdhdr1st(stringstream &buf);
  bool rdhdrs();
  void rduri(string &uri);

  // Request header jump table.
  static size_t const _num_header = 47; // =(
  static void (Derived::*_hdrjmp[_num_header])(stringstream &);
  static Scheduler *sch;
  static size_t const cbufsz = 1<<12; // =(


  // General-purpose parsing buffers.
  stringstream parsebuf, tmpbuf;
  uint8_t cbuf[cbufsz];
  
  // Some applications need to save the request for possible forwarding.
  static bool savereq; // should be const; todo
  list<string> reqhdrs;
  string reqbody;

  // Pointers to buffers involved in writing

  // Points to buffer containing response headers (never NULL)
  uint8_t const *resp_hdrs;
  size_t resp_hdrs_sz;

  // Points to buffer containing response body (can be NULL)
  uint8_t const *resp_body;
  size_t resp_body_sz;

  /* Before we start writing a response, we set
   * out = resp_hdrs and outsz = resp_hdrs_sz. After we have finished writing
   * the headers, we set
   * out = resp_body and outsz = resp_body_sz. */
  uint8_t const *out;
  size_t outsz;

  status stat; // Status code we'll return to client
};

// Now THIS is a declaration!
template<class Derived>
void (Derived::* HTTP_Work<Derived>::_hdrjmp[_num_header])(stringstream &);

template<class Derived>
Scheduler *HTTP_Work<Derived>::sch;

template<class Derived>
bool HTTP_Work<Derived>::savereq;

template<class Derived>
HTTP_Work<Derived>::HTTP_Work(int fd, Work::mode m)
  : Work(fd, m), progress(nothing_done), stat(OK), reqbodysz(0)
{
}

template<class Derived>
void HTTP_Work<Derived>::setsch(Scheduler *_sch)
{
  sch = _sch;
}

/* RFC 2616 sec. 5: Request = Request-Line
 * *((general-header | request-header | entity-header) CRLF)
 * CRLF [message-body] */
template<class Derived>
bool HTTP_Work<Derived>::rdhdrs()
{
  string line;

  // Get a line until there are no more lines, or we hit the empty line.
  while (getline(parsebuf, line, '\r') && line.length() > 0) {
    /* If the line isn't properly terminated, save it and report that we
     * need more text from the client in order to parse. */
    if (parsebuf.peek() != '\n') {
      parsebuf.clear();
      parsebuf.str(line);
      return false;
    }
    parsebuf.ignore(); // Ignore the \n (the \r is already consumed)
    _LOG_DEBUG("parse_header %s", line.c_str());
    if (savereq)
      reqhdrs.push_back(line);

    tmpbuf.str("");
    tmpbuf.clear();
    tmpbuf << line;

    if (progress == nothing_done) {
      rdhdr1st(tmpbuf);
      progress = req1st_done;
    }
    else {
      // Arbitrary initialization just to avoid valgrind complaints.
      header h = Accept;
      try {
	tmpbuf >> h;
      }
      catch (HTTP_Parse_Err e) {
	if (e.stat == Not_Implemented)
	  continue; // Silently ignore headers we don't implement.
	else {
	  stat = e.stat;
	  return true; // Immediately stop parsing.
	}
      }
      // Execute the callback for this header, if it exists.
      if (_hdrjmp[h] != NULL)
	(mem_fun(_hdrjmp[h]))(static_cast<Derived *>(this), tmpbuf);
    }
  }
  parsebuf.clear();
  // We got to the empty line.
  if (line.length() == 0 && parsebuf.peek() == '\n') {
    progress = reqhdrs_done;
    return true;
  } else {
    _LOG_DEBUG("line not properly terminated: %s", line.c_str());
    parsebuf.str(line);
    return false;
  }
}

template<class Derived>
void HTTP_Work<Derived>::operator()()
{
  int err;
  switch(m) {
  case Work::read:
    err = (progress != reqhdrs_done)
      ? rduntil(parsebuf, cbuf, cbufsz) // Read headers
      : rduntil(parsebuf, cbuf, cbufsz, reqbodysz); // Read body
    if (err != 0 && err != EAGAIN && err != EWOULDBLOCK)
      throw SocketErr("read", err);
    /* TODO: right now we schedule a write immediately after the reading is
     * complete. But when we add more asynchronous/nonblocking I/O on
     * the server side (e.g. wait for a CGI program to return), we would want
     * the write to be scheduled only when all the resources are ready. */
    if (progress != reqhdrs_done) {
      if (rdhdrs()) {
	// If there's no body to read, schedule a write.
	if (reqbodysz == 0) {
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
      if (progress != resphdrs_done) {
	progress = resphdrs_done;
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
template<class Derived>
void HTTP_Work<Derived>::rdhdr1st(stringstream &buf)
{
  _LOG_DEBUG("rdhdr1st %s", buf.str().c_str());
  buf >> meth;
  string uri;
  buf >> uri;
  rduri(uri);
  string version;
  buf >> version;
  if (version != HTTP_Version)
    throw HTTP_Parse_Err(HTTP_Version_Not_Supported);
}

template<class Derived>
void HTTP_Work<Derived>::rduri(string &uri)
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

template<class Derived>
void HTTP_Work<Derived>::prepare_response()
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

#endif // HTTP_WORK_HPP
