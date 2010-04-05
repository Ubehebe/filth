#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>

#include "FindWork_prealloc.hpp"
#include "HTTP_constants.hpp"
#include "LockFreeQueue.hpp"
#include "HTTP_Cache.hpp"
#include "Magic_nr.hpp"
#include "Scheduler.hpp"
#include "Time_nr.hpp"
#include "Work.hpp"
#include "Workmap.hpp"

class HTTP_Work : public Work
{
  friend class FindWork_prealloc<HTTP_Work>;
  friend class HTTP_FindWork;
  HTTP_Work(HTTP_Work const&);
  HTTP_Work &operator=(HTTP_Work const&);

  static LockFreeQueue<void *> store; // For operator new/delete
  static size_t const rdbufsz = 1<<12; // 4K
  static Scheduler *sch;
  static HTTP_Cache *cache;
  static Workmap *st;

private:
  // Internal state.
  Time_nr date; // For doing timestamps
  Magic_nr MIME; // For doing MIME type lookups
  char rdbuf[rdbufsz]; // General-purpose raw buffer
  std::string path; // Path to resource
  std::string query; // The stuff after the "?" in a URI; to pass to resource
  std::stringstream pbuf; // Buffer to use in parsing

  /* The raw request is stored as a list of strings in case we have to
   * forward it on. */
  typedef std::list<std::string> req_type;
  req_type req; 

  // Pointers to buffers involved in writing

  // Points to buffer containing response headers (never NULL)
  char const *resp_hdrs;
  size_t resp_hdrs_sz;

  // Points to buffer containing response body (can be NULL)
  char const *resp_body;
  size_t resp_body_sz;

  /* Before we start writing a response, we set
   * out = resp_hdrs and outsz = resp_hdrs_sz. After we have finished writing
   * the headers, we set
   * out = resp_body and outsz = resp_body_sz. */
  char const *out;
  size_t outsz;

  // Know when to switch from headers to body.
  bool hdrs_done;

  HTTP_constants::status stat; // Status code we'll return to client
  HTTP_constants::method meth; // Method (GET, POST, etc.)

  /* Stuff reported by the _client_ in the request headers.
   * TODO: this stuff is more useful when we think about proxies, right? */
  size_t cl_sz, cl_max_fwds;
  std::string cl_content_type, cl_expires, cl_from,
				   cl_host, cl_pragma, cl_referer, cl_user_agent;
  
  bool rdlines();
  void parse_req();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  void negotiate_content();
  std::string &uri_hex_escape(std::string &uri);
  
public:
  void operator()();
  void *operator new(size_t sz);
  void operator delete(void *work);
  HTTP_Work(int fd, Work::mode m);
  ~HTTP_Work();
};


#endif // HTTP_WORK_HPP
