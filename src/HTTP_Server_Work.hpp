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

class HTTP_Server_Work : public HTTP_Work
{
public:
  HTTP_Server_Work(int fd, Work::mode m);
  ~HTTP_Server_Work();

  void *operator new(size_t sz);
  void operator delete(void *work);

  private:
  void browsehdrs();
  void prepare_response();

  string path, query;
  method meth;

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

  // Stuff reported by the _client_ in the request headers.
  size_t cl_max_fwds;
  HTTP_constants::content_coding cl_accept_enc;
};


#endif // HTTP_SERVER_WORK_HPP
