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
#include "MIME_FileCache.hpp"
#include "Scheduler.hpp"
#include "Time_nonthreadsafe.hpp"
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
  static FileCache *cache;
  static Workmap *st;

private:
  // Internal state.
  Time_nonthreadsafe date;
  char rdbuf[rdbufsz]; // Buffer to use to read from client...and response??
  char const *MIME_type;
  std::string path; // Path to resource
  std::string query; // The stuff after the "?" in a URI; to pass to resource
  char const *resource; // Raw pointer to resource contents
  std::stringstream pbuf; // Buffer to use in parsing
  std::list<std::string> req; // Store the request as a list of lines.
  HTTP_constants::status stat; // Status code we'll return to client
  HTTP_constants::method meth; // Method (GET, POST, etc.)

  /* Stuff reported by the _client_ in the request headers.
   * TODO: this stuff is more useful when we think about proxies, right? */
  size_t cl_sz, cl_max_fwds;
  std::string cl_content_type, cl_expires, cl_from,
				   cl_host, cl_pragma, cl_referer, cl_user_agent;

  size_t resourcesz; // Size of resource (for static only??)
  bool resp_headers_done;
  char const *outgoing_offset;
  size_t outgoing_offset_sz;

  
  void prepare_resp();
  bool rdlines();
  void parse_req();
  void parse_req_line(std::string &line);
  void parse_header(std::string &line);
  void parse_uri(std::string &uri);
  std::string &uri_hex_escape(std::string &uri);
  
public:
  void operator()();
  void *operator new(size_t sz);
  void operator delete(void *work);
  HTTP_Work(int fd, Work::mode m);
  ~HTTP_Work();
};


#endif // HTTP_WORK_HPP
