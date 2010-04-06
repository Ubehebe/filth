#ifndef HTTP_WORK_HPP
#define HTTP_WORK_HPP

#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>

#include "Compressor.hpp"
#include "FindWork_prealloc.hpp"
#include "HTTP_constants.hpp"
#include "LockFreeQueue.hpp"
#include "HTTP_Cache.hpp"
#include "Magic.hpp"
#include "Scheduler.hpp"
#include "Time.hpp"
#include "Work.hpp"
#include "Workmap.hpp"

class HTTP_Work : public Work
{
public:
  void operator()();
  void *operator new(size_t sz);
  void operator delete(void *work);
  HTTP_Work(int fd, Work::mode m);
  ~HTTP_Work();
private:

  friend class FindWork_prealloc<HTTP_Work>;
  friend class HTTP_FindWork;
  HTTP_Work(HTTP_Work const&);
  HTTP_Work &operator=(HTTP_Work const&);

  // State that is the same for all work objects.
  static LockFreeQueue<void *> store; // For operator new/delete
  static size_t const rdbufsz = 1<<12; // 4K
  static Scheduler *sch;
  static HTTP_Cache *cache;
  static Workmap *st;
  static Time *date;
  static Compressor *compress;
  static Magic *MIME;

  // Internal state (Work-specific, NOT Worker-specific!)

  char rdbuf[rdbufsz]; // General-purpose raw buffer
  std::string path; // Path to resource
  std::string query; // The stuff after the "?" in a URI; to pass to resource
  std::stringstream pbuf; // Buffer to use in parsing

  /* The raw request headers are stored as a list of strings in case 
   * we have to forward them on. */
  typedef std::list<std::string> req_type;
  req_type req; 
  // The request body (if any) is stored as-is.
  std::string req_body;

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
  bool inhdrs_done, outhdrs_done;


  HTTP_constants::status stat; // Status code we'll return to client
  HTTP_constants::method meth; // Method (GET, POST, etc.)

  /* Stuff reported by the _client_ in the request headers.
   * TODO: this stuff is more useful when we think about proxies, right? */
  size_t cl_sz, cl_max_fwds;
  std::string cl_content_type, cl_expires, cl_from,
				   cl_host, cl_pragma, cl_referer, cl_user_agent;
  
  bool rdlines();
  void rdbody();
  void parse_req();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  void negotiate_content();
  std::string &uri_hex_escape(std::string &uri);
  

};


#endif // HTTP_WORK_HPP
