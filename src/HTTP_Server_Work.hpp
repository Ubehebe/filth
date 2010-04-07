#ifndef HTTP_SERVER_WORK_HPP
#define HTTP_SERVER_WORK_HPP

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
#include "HTTP_Work.hpp"
#include "Magic.hpp"
#include "Scheduler.hpp"
#include "Time.hpp"
#include "Work.hpp"
#include "Workmap.hpp"

class HTTP_Server_Work : public HTTP_Work<HTTP_Server_Work>
{
public:
  HTTP_Server_Work(int fd, Work::mode m);
  ~HTTP_Server_Work();
  void prepare_response();
  void *operator new(size_t sz);
  void operator delete(void *work);

  /* Dispatch functions for the various headers. Any header not mentioned
   * here is ignored. */
  void Accept_Encoding(std::stringstream &buf);
  void Connection(std::stringstream &buf);
  void Content_Length(std::stringstream &buf);
  void Content_Type(std::stringstream &buf);
  void Expect(std::stringstream &buf);
  void Expires(std::stringstream &buf);
  void From(std::stringstream &buf);
  void Host(std::stringstream &buf);
  void Max_Forwards(std::stringstream &buf);
  void Pragma(std::stringstream &buf);
  void Referer(std::stringstream &buf);
  void User_Agent(std::stringstream &buf);

  private:

  friend class FindWork_prealloc<HTTP_Server_Work>;
  friend class HTTP_FindWork;
  HTTP_Server_Work(HTTP_Server_Work const&);
  HTTP_Server_Work &operator=(HTTP_Server_Work const&);

  // State that is the same for all work objects.
  static LockFreeQueue<void *> store; // For operator new/delete
  static HTTP_Cache *cache;
  static Workmap *st;
  static Time *date;
  static Magic *MIME;

  // Internal state (Work-specific, NOT Worker-specific!)

  /* Stuff reported by the _client_ in the request headers.
   * TODO: this stuff is more useful when we think about proxies, right? */
  size_t cl_max_fwds;
  std::string cl_content_type, cl_expires, cl_from,
				   cl_host, cl_pragma, cl_referer, cl_user_agent;
  HTTP_constants::content_coding cl_accept_enc;
  
  std::string &uri_hex_escape(std::string &uri);
};


#endif // HTTP_SERVER_WORK_HPP
