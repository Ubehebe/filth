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
#include "Magic_nr.hpp"
#include "Scheduler.hpp"
#include "Time_nr.hpp"
#include "Workmap.hpp"

class HTTP_Server_Work : public HTTP_Work
{
public:
  HTTP_Server_Work(int fd, Work::mode m);
  ~HTTP_Server_Work();
  
  void *operator new(size_t sz);
  void operator delete(void *work);

private:
  void browse_req(HTTP_Work::req_hdrs_type &req_hdrs,
		  std::string const &req_body);
  bool cache_get(std::string &path, HTTP_CacheEntry *&c);
  bool cache_put(std::string &path, HTTP_CacheEntry *c, size_t sz);
  void prepare_response(stringstream &hdrs,
			uint8_t const *&body, size_t &bodysz);
  void on_parse_err(status &s, stringstream &hdrs,
		    uint8_t const *&body, size_t &bodysz);
  void reset();

  friend class FindWork_prealloc<HTTP_Server_Work>;
  friend class HTTP_FindWork;
  friend class HTTP_Worker;
  HTTP_Server_Work(HTTP_Server_Work const&);
  HTTP_Server_Work &operator=(HTTP_Server_Work const&);

  // State that is the same for all work objects.
  static LockFreeQueue<void *> store; // For operator new/delete
  static HTTP_Cache *cache;
  static Workmap *st;

  // State imbued from the Worker object that we use.
  Time_nr *date;
  Magic_nr *MIME;

  // Stuff reported by the client in the request headers.

  string path, query;
  method meth;

  /* Normally the response either comes straight from the cache or
   * comes from disk and is left in the cache for later use, but if neither of
   * these is true we need to remember to delete it ourselves. */
  bool resp_is_cached;
  HTTP_CacheEntry *c;
  

  struct cc
  {
    enum opts {
      no_cache = 1<<0,
      no_store = 1<<1,
      no_transform = 1<<2,
      only_if_cached = 1<<3,
      use_max_age = 1<<4,
      use_min_fresh = 1<<5,
      use_max_stale = 1<<6 };
    uint8_t flags; // Remember to change if more than 8 flags :)
    time_t max_age, min_fresh, max_stale;
    cc() : flags(0), max_age(0), min_fresh(0), max_stale(0) {}
    inline void set(opts o) { flags |= o; }
    inline bool isset(opts o) { return flags & o; }
    inline void clear() { flags = 0; }
  };
  cc cl_cache_control;
  HTTP_constants::content_coding cl_accept_enc;
  size_t cl_max_fwds;
};


#endif // HTTP_SERVER_WORK_HPP
